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
	auto result = QColor();
	result.setRedF(a.redF() + percent * (b.redF() - a.redF()));
	result.setGreenF(a.greenF() + percent * (b.greenF() - a.greenF()));
	result.setBlueF(a.blueF() + percent * (b.blueF() - a.blueF()));
	return result;
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

[[nodiscard]] QGradientStops ParseColorStops(
		const JsonArray &data,
		int colorPoints) {
	auto result = QGradientStops();
	result.reserve(colorPoints);
	for (auto i = 0; i != colorPoints; ++i) {
		auto color = QColor();
		color.setRedF(ValueAt(data, i * 4 + 1));
		color.setGreenF(ValueAt(data, i * 4 + 2));
		color.setBlueF(ValueAt(data, i * 4 + 3));
		const auto time = ValueAt(data, i * 4);
		if (i > 0 && result[i - 1].first > time) {
			qWarning() << "Bad timing in gradient points.";
			return QGradientStops();
		}
		result.push_back({ time, color });
	}
	return result;
}

[[nodiscard]] QGradientStops Pass(QGradientStops &list) {
	return std::exchange(list, QGradientStops());
}

[[nodiscard]] QGradientStops InterpolateAlphaValues(
		QGradientStops list,
		int from,
		int to) {
	if (to == list.size() || from < 0) {
		const auto value = (from >= 0)
			? list[from].second.alphaF()
			: (to < list.size())
			? list[to].second.alphaF()
			: 1.;
		for (auto i = from + 1; i != to; ++i) {
			list[i].second.setAlphaF(value);
		}
	} else {
		const auto fromTime = list[from].first;
		const auto fromAlpha = list[from].second.alphaF();
		const auto toTime = list[to].first;
		const auto toAlpha = list[to].second.alphaF();
		for (auto i = from + 1; i != to; ++i) {
			const auto percent = (toTime > fromTime)
				? ((list[i].first - fromTime) / (toTime - fromTime))
				: 1.;
			list[i].second.setAlphaF(
				fromAlpha + percent * (toAlpha - fromAlpha));
		}
	}
	return list;
}

QGradientStops AddOpacityStops(
		QGradientStops list,
		const JsonArray &data,
		int colorPoints) {
	const auto opacityPoints = (data.size() - colorPoints * 4) / 2;
	if (!opacityPoints) {
		return list;
	}
	list.reserve(colorPoints + opacityPoints);

	auto alphaSetTill = -1;
	const auto insertOpacityPoint = [&](
			int index,
			double time,
			double opacity) {
		auto color = (index == 0)
			? list[index].second
			: (index == list.size())
			? list[index - 1].second
			: InterpolateColor(list[index - 1], list[index], time);
		color.setAlphaF(opacity);
		list.insert(list.begin() + index, { time, color });
		list = InterpolateAlphaValues(Pass(list), alphaSetTill, index);
		alphaSetTill = index;
	};
	const auto addOpacityPoint = [&](double time, double opacity) {
		for (auto i = 0; i != list.size(); ++i) {
			if (alphaSetTill < i && qFuzzyCompare(list[i].first, time)) {
				list[i].second.setAlphaF(opacity);
				list = InterpolateAlphaValues(Pass(list), alphaSetTill, i);
				alphaSetTill = i;
				return;
			} else if (list[i].first > time) {
				insertOpacityPoint(i, time, opacity);
				return;
			}
		}
		insertOpacityPoint(list.size(), time, opacity);
	};
	const auto shift = colorPoints * 4;
	for (auto i = 0; i != opacityPoints; ++i) {
		const auto time = ValueAt(data, shift + i * 2);
		const auto opacity = ValueAt(data, shift + i * 2 + 1);
		if (i > 0 && list[alphaSetTill].first > time) {
			qWarning() << "Bad opacity timing in gradient points.";
			return QGradientStops();
		}
		addOpacityPoint(time, opacity);
	}
	const auto size = list.size();
	return InterpolateAlphaValues(Pass(list), alphaSetTill, size);
}

QGradientStops ParseStaticGradient(const JsonArray &data, int colorPoints) {
	if (colorPoints <= 0 || data.size() < colorPoints * 4) {
		qWarning() << "Bad data in gradient points.";
		return QGradientStops();
	}
	auto result = ParseColorStops(data, colorPoints);
	if (result.isEmpty()) {
		return QGradientStops();
	}
	return AddOpacityStops(Pass(result), data, colorPoints);
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
, m_colors(other.m_colors) {
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
		m_gradient->setStops(ParseStaticGradient(data, colorPoints));
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
	if (data.empty()) {
		qWarning() << "Bad data in animated gradient points.";
		return;
	}
	const auto start = data.at(0).toObject().value("s").toArray();
	if (start.empty() || start.size() < colorPoints * 4) {
		qWarning() << "Bad data in animated gradient points.";
		return;
	}
	const auto opacityPoints = (start.size() - colorPoints * 4) / 2;

}

void BMGFill::updateProperties(int frame) {
	QGradient::Type type = gradientType();
	if (type != QGradient::LinearGradient
		&& type != QGradient::RadialGradient) {
		return;
	}

	m_startPoint.update(frame);
	m_endPoint.update(frame);
	m_highlightLength.update(frame);
	m_highlightAngle.update(frame);
	m_opacity.update(frame);
	QList<BMProperty<QVector4D>>::iterator colorIt = m_colors.begin();
	while (colorIt != m_colors.end()) {
		(*colorIt).update(frame);
		++colorIt;
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

void BMGFill::setGradient() {
	switch (gradientType()) {
	case QGradient::LinearGradient: {
		QLinearGradient *g = static_cast<QLinearGradient*>(m_gradient);
		g->setStart(m_startPoint.value());
		g->setFinalStop(m_endPoint.value());
		break;
	}
	case QGradient::RadialGradient: {
		QRadialGradient *g = static_cast<QRadialGradient*>(m_gradient);
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
