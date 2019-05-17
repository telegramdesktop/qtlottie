/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the lottie-qt module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "bmgfill.h"

#include "renderer.h"

#include <QLinearGradient>
#include <QRadialGradient>
#include <QtMath>
#include <QColor>

namespace Lottie {
namespace {

[[nodiscard]] double ValueAt(const JsonArray &data, int index) {
	return std::clamp(data.at(index).toDouble(), 0., 1.);
}

[[nodiscard]] QColor InterpolateColor(QColor a, QColor b, double percent) {
	return QColor::fromRgbF(
		a.redF() + percent * (b.redF() - a.redF()),
		a.greenF() + percent * (b.greenF() - a.greenF()),
		a.blueF() + percent * (b.blueF() - a.blueF()));
}

[[nodiscard]] QColor InterpolateColor(
		QGradientStop a,
		QGradientStop b,
		double time) {
	const auto percent = (time > a.first)
		? ((time - a.first) / (b.first - a.first))
		: 1.;
	return InterpolateColor(a.second, b.second, percent);
}

class StopsBuilder {
public:
	StopsBuilder(const JsonArray &data, int colorPoints);
	StopsBuilder(
		const QVector<BMProperty<QVector4D>> &colorStops,
		const QVector<BMProperty<QSizeF>> &opacityStops);

	QGradientStops result();

private:
	bool validateInput();

	void parseColorStops(const JsonArray &data);
	void computeColorStops(const QVector<BMProperty<QVector4D>> &stops);
	void addOpacityStops(const JsonArray &data);
	void addOpacityStops(const QVector<BMProperty<QSizeF>> &stops);

	bool pushColorStop(QGradientStop stop);
	void insertOpacityPoint(int index, double time, double opacity);
	bool addOpacityPoint(double time, double opacity);
	void interpolateAlphaTill(int till);

	QGradientStops _result;
	int _colorStops = 0;
	int _opacityStops = 0;
	int _alphaSetTill = -1;
	bool _error = false;

};

StopsBuilder::StopsBuilder(const JsonArray &data, int colorPoints)
: _colorStops(colorPoints)
, _opacityStops((data.size() - 4 * colorPoints) / 2) {
	if (data.size() < 4 * colorPoints) {
		qWarning() << "Bad data in gradient points.";
		_error = true;
	} else if (validateInput()) {
		parseColorStops(data);
		addOpacityStops(data);
	}
}

StopsBuilder::StopsBuilder(
	const QVector<BMProperty<QVector4D>> &colorStops,
	const QVector<BMProperty<QSizeF>> &opacityStops)
: _colorStops(colorStops.size())
, _opacityStops(opacityStops.size()) {
	if (validateInput()) {
		computeColorStops(colorStops);
		addOpacityStops(opacityStops);
	}
}

QGradientStops StopsBuilder::result() {
	return _error ? QGradientStops() : std::move(_result);
}

bool StopsBuilder::validateInput() {
	if (_colorStops <= 0 || _opacityStops < 0) {
		qWarning() << "Bad data in gradient points.";
		_error = true;
		return false;
	}
	_result.reserve(_colorStops + _opacityStops);
	return true;
}

void StopsBuilder::parseColorStops(const JsonArray &data) {
	for (auto i = 0; i != _colorStops; ++i) {
		const auto color = QColor::fromRgbF(
			ValueAt(data, i * 4 + 1),
			ValueAt(data, i * 4 + 2),
			ValueAt(data, i * 4 + 3));
		const auto time = ValueAt(data, i * 4);
		if (!pushColorStop({ time, color })) {
			break;
		}
	}
}

void StopsBuilder::computeColorStops(
		const QVector<BMProperty<QVector4D>> &stops) {
	for (const auto &stop : stops) {
		const auto value = stop.value();
		const auto color = QColor::fromRgbF(
			std::clamp(double(value.x()), 0., 1.),
			std::clamp(double(value.y()), 0., 1.),
			std::clamp(double(value.z()), 0., 1.));
		const auto time = std::clamp(double(value.w()), 0., 1.);
		if (!pushColorStop({ time, color })) {
			break;
		}
	}
}

void StopsBuilder::addOpacityStops(const JsonArray &data) {
	if (!_opacityStops) {
		return;
	}
	const auto shift = _colorStops * 4;
	for (auto i = 0; i != _opacityStops; ++i) {
		const auto time = ValueAt(data, shift + i * 2);
		const auto opacity = ValueAt(data, shift + i * 2 + 1);
		if (!addOpacityPoint(time, opacity)) {
			break;
		}
	}
	interpolateAlphaTill(_result.size());
}

void StopsBuilder::addOpacityStops(
		const QVector<BMProperty<QSizeF>> &stops) {
	for (const auto &stop : stops) {
		const auto value = stop.value();
		const auto time = std::clamp(value.width(), 0., 1.);
		const auto opacity = std::clamp(value.height(), 0., 1.);
		if (!addOpacityPoint(time, opacity)) {
			break;
		}
	}
	interpolateAlphaTill(_result.size());
}

void StopsBuilder::insertOpacityPoint(
		int index,
		double time,
		double opacity) {
	auto color = (index == 0)
		? _result[index].second
		: (index == _result.size())
		? _result[index - 1].second
		: InterpolateColor(_result[index - 1], _result[index], time);
	color.setAlphaF(opacity);
	_result.insert(_result.begin() + index, { time, color });
	interpolateAlphaTill(index);
}

bool StopsBuilder::addOpacityPoint(double time, double opacity) {
	if (_alphaSetTill >= 0 && _result[_alphaSetTill].first > time) {
		qWarning() << "Bad opacity timing in gradient points.";
		_error = true;
		return false;
	}
	for (auto i = _alphaSetTill + 1; i != _result.size(); ++i) {
		if (qFuzzyCompare(_result[i].first, time)) {
			_result[i].second.setAlphaF(opacity);
			interpolateAlphaTill(i);
			return true;
		} else if (_result[i].first > time) {
			insertOpacityPoint(i, time, opacity);
			return true;
		}
	}
	insertOpacityPoint(_result.size(), time, opacity);
	return true;
}

bool StopsBuilder::pushColorStop(QGradientStop stop) {
	if (!_result.empty() && _result.back().first > stop.first) {
		qWarning() << "Bad timing in gradient points.";
		_error = true;
		return false;
	}
	_result.push_back(stop);
	return true;
}

void StopsBuilder::interpolateAlphaTill(int till) {
	const auto from = _alphaSetTill;
	if (from < 0 && till == _result.size()) {
		// All values are 1. already.
	} else if (from < 0 || till == _result.size()) {
		const auto value = (from >= 0)
			? _result[from].second.alphaF()
			: _result[till].second.alphaF();
		for (auto i = from + 1; i != till; ++i) {
			_result[i].second.setAlphaF(value);
		}
	} else {
		const auto fromTime = _result[from].first;
		const auto fromAlpha = _result[from].second.alphaF();
		const auto tillTime = _result[till].first;
		const auto tillAlpha = _result[till].second.alphaF();
		for (auto i = from + 1; i != till; ++i) {
			const auto percent = (tillTime > fromTime)
				? ((_result[i].first - fromTime) / (tillTime - fromTime))
				: 1.;
			_result[i].second.setAlphaF(
				fromAlpha + percent * (tillAlpha - fromAlpha));
		}
	}
	_alphaSetTill = till;
}

} // namespace

BMGFill::BMGFill(BMBase *parent) : BMShape(parent) {
}

BMGFill::BMGFill(BMBase *parent, const BMGFill &other)
: BMShape(parent, other)
, m_opacity(other.m_opacity)
, m_startPoint(other.m_startPoint)
, m_endPoint(other.m_endPoint)
, m_highlightLength(other.m_highlightLength)
, m_highlightAngle(other.m_highlightAngle)
, m_colorStops(other.m_colorStops)
, m_opacityStops(other.m_opacityStops) {
	if (other.m_gradient) {
		if (other.gradientType() == QGradient::LinearGradient) {
			m_gradient = new QLinearGradient(*static_cast<QLinearGradient*>(other.m_gradient));
		} else if (other.gradientType() == QGradient::RadialGradient) {
			m_gradient = new QRadialGradient(*static_cast<QRadialGradient*>(other.m_gradient));
		}
	}
}

BMGFill::~BMGFill() {
	if (m_gradient) {
		delete m_gradient;
	}
}

BMBase *BMGFill::clone(BMBase *parent) const {
	return new BMGFill(parent, *this);
}

BMGFill::BMGFill(BMBase *parent, const JsonObject &definition)
: BMShape(parent) {
	BMBase::parse(definition);
	if (m_hidden) {
		return;
	}

	int type = definition.value("t").toInt();
	switch (type) {
	case 1:
		m_gradient = new QLinearGradient;
		break;
	case 2:
		m_gradient = new QRadialGradient;
		break;
	default:
		qWarning() << "Unknown gradient fill type.";
	}

	const auto gradient = definition.value("g").toObject();
	const auto data = gradient.value("k").toObject().value("k").toArray();
	const auto colorPoints = gradient.value("p").toInt();
	if (data.empty() || colorPoints <= 0) {
		qWarning() << "Bad data in gradient points.";
	} else if (data.at(0).isObject()) {
		parseAnimatedGradient(data, colorPoints);
	} else if (m_gradient) {
		m_gradient->setStops(StopsBuilder(data, colorPoints).result());
	}

	const auto opacity = definition.value("o").toObject();
	m_opacity.construct(opacity);

	const auto startPoint = definition.value("s").toObject();
	m_startPoint.construct(startPoint);

	const auto endPoint = definition.value("e").toObject();
	m_endPoint.construct(endPoint);

	const auto highlight = definition.value("h").toObject();
	m_highlightLength.construct(highlight);

	const auto angle = definition.value("a").toObject();
	m_highlightAngle.construct(angle);

	m_highlightAngle.setValue(0.0);
}

void BMGFill::parseAnimatedGradient(const JsonArray &data, int colorPoints) {
	if (data.empty() || colorPoints <= 0) {
		qWarning() << "Bad data in animated gradient points.";
		return;
	}
	auto colors = std::vector<ConstructAnimatedData<QVector4D>>();
	auto opacities = std::vector<ConstructAnimatedData<QSizeF>>();

	for (const auto &element : data) {
		const auto keyframe = element.toObject();

		const auto hold = (keyframe.value("h").toInt() == 1);
		const auto startFrame = keyframe.value("t").toInt();
		const auto easingIn = ParseEasingInOut(keyframe.value("i").toObject());
		const auto easingOut = ParseEasingInOut(keyframe.value("o").toObject());

		const auto startValue = keyframe.value("s").toArray();
		const auto endValue = keyframe.value("e").toArray();

		if (!startValue.empty()
			&& !colors.empty()
			&& startValue.size() != colors.size() * 4 + opacities.size() * 2) {
			qWarning() << "Bad data in animated gradient points.";
			return;
		}
		const auto opacityPoints = startValue.empty()
			? int(opacities.size())
			: int((startValue.size() - 4 * colorPoints) / 2);
		if (!startValue.empty()
			&& startValue.size() < 4 * colorPoints) {
			qWarning() << "Bad data in shape.";
			return;
		}
		if (colors.empty()) {
			colors.resize(colorPoints);
			for (auto &color : colors) {
				color.keyframes.reserve(data.size());
			}
		}
		if (opacityPoints > 0 && opacities.empty()) {
			opacities.resize(opacityPoints);
			for (auto &opacity : opacities) {
				opacity.keyframes.reserve(data.size());
			}
		}

		for (auto i = 0; i != colorPoints; ++i) {
			auto color = ConstructKeyframeData<QVector4D>();
			color.hold = hold;
			color.startFrame = startFrame;

			if (!startValue.empty()) {
				color.easingIn = easingIn;
				color.easingOut = easingOut;
				color.startValue = QVector4D(
					startValue.at(i * 4 + 1).toDouble(), // time, r, g, b, ..
					startValue.at(i * 4 + 2).toDouble(),
					startValue.at(i * 4 + 3).toDouble(),
					startValue.at(i * 4).toDouble());
				color.endValue = QVector4D(
					endValue.at(i * 4 + 1).toDouble(),
					endValue.at(i * 4 + 2).toDouble(),
					endValue.at(i * 4 + 3).toDouble(),
					endValue.at(i * 4).toDouble());
			}

			colors[i].keyframes.push_back(std::move(color));
		}

		const auto shift = colorPoints * 4;
		for (auto i = 0; i != opacityPoints; ++i) {
			auto opacity = ConstructKeyframeData<QSizeF>();
			opacity.hold = hold;
			opacity.startFrame = startFrame;

			if (!startValue.empty()) {
				opacity.easingIn = easingIn;
				opacity.easingOut = easingOut;
				opacity.startValue = QSizeF(
					startValue.at(shift + i * 2).toDouble(),
					startValue.at(shift + i * 2 + 1).toDouble());
				opacity.endValue = QSizeF(
					endValue.at(shift + i * 2).toDouble(),
					endValue.at(shift + i * 2 + 1).toDouble());
			}

			opacities[i].keyframes.push_back(std::move(opacity));
		}
	}
	m_colorStops.reserve(colors.size());
	for (const auto &color : colors) {
		m_colorStops.push_back({});
		m_colorStops.back().constructAnimated(color);
	}
	m_opacityStops.reserve(opacities.size());
	for (const auto &opacity : opacities) {
		m_opacityStops.push_back({});
		m_opacityStops.back().constructAnimated(opacity);
	}
}

void BMGFill::updateProperties(int frame) {
	if (!m_gradient) {
		return;
	}

	m_startPoint.update(frame);
	m_endPoint.update(frame);
	m_highlightLength.update(frame);
	m_highlightAngle.update(frame);
	m_opacity.update(frame);
	for (auto &stop : m_colorStops) {
		stop.update(frame);
	}
	for (auto &stop : m_opacityStops) {
		stop.update(frame);
	}
	setGradient();
}

void BMGFill::render(Renderer &renderer, int frame) const {
	renderer.render(*this);
}

QGradient *BMGFill::value() const {
	return m_gradient;
}

QGradient::Type BMGFill::gradientType() const {
	if (m_gradient) {
		return m_gradient->type();
	} else {
		return QGradient::NoGradient;
	}
}

QPointF BMGFill::startPoint() const {
	return m_startPoint.value();
}

QPointF BMGFill::endPoint() const {
	return m_endPoint.value();
}

qreal BMGFill::highlightLength() const {
	return m_highlightLength.value();
}

qreal BMGFill::highlightAngle() const {
	return m_highlightAngle.value();
}

qreal BMGFill::opacity() const {
	return m_opacity.value();
}

void BMGFill::setGradientStops() {
	m_gradient->setStops(
		StopsBuilder(m_colorStops, m_opacityStops).result());
}

void BMGFill::setGradient() {
	if (!m_colorStops.empty()) {
		setGradientStops();
	}

	switch (gradientType()) {
	case QGradient::LinearGradient: {
		const auto g = static_cast<QLinearGradient*>(m_gradient);
		g->setStart(m_startPoint.value());
		g->setFinalStop(m_endPoint.value());
		break;
	}
	case QGradient::RadialGradient: {
		const auto g = static_cast<QRadialGradient*>(m_gradient);
		qreal dx = m_endPoint.value().x() - m_startPoint.value().x();
		qreal dy = m_endPoint.value().y() - m_startPoint.value().y();
		qreal radius = qSqrt(dx * dx +  dy * dy);
		g->setCenter(m_startPoint.value());
		g->setCenterRadius(radius);

		qreal angle = qAtan2(dy, dx) + qDegreesToRadians(m_highlightAngle.value());
		qreal percent = std::clamp(m_highlightLength.value(), -0.99, 0.99);
		qreal dist = radius * percent;
		g->setFocalPoint(g->center() + dist * QPointF(qCos(angle), qSin(angle)));
		g->setFocalRadius(0.);
		break;
	}
	default:
		break;
	}
}

} // namespace Lottie
