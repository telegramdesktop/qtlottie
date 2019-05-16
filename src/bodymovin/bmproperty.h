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
#pragma once

#include "beziereasing.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QPointF>
#include <QSizeF>
#include <QVector4D>
#include <QtMath>
#include <vector>

namespace Lottie {

template <typename T>
struct EasingSegmentBasic {
	double startFrame = 0;
	double endFrame = 0;
	T startValue = T();
	T endValue = T();
	BezierEasing easing;
};

template <typename T>
struct EasingSegment : EasingSegmentBasic<T> {
};

template <>
struct EasingSegment<QPointF> : EasingSegmentBasic<QPointF> {
	struct BezierPoint {
		QPointF point;
		double length = 0.;
	};

	double bezierLength = 0.;
	QVector<BezierPoint> bezierPoints;
};

template <typename T>
struct ConstructKeyframeDataBasic {
	T startValue = T();
	T endValue = T();
	double startFrame = 0.;
	QPointF easingIn;
	QPointF easingOut;
	bool hold = false;
};

template <typename T>
struct ConstructKeyframeData : ConstructKeyframeDataBasic<T> {
};

template <>
struct ConstructKeyframeData<QPointF> : ConstructKeyframeDataBasic<QPointF> {
	QPointF tangentIn;
	QPointF tangentOut;
};

template <typename T>
struct ConstructAnimatedData {
	std::vector<ConstructKeyframeData<T>> keyframes;
};

inline QPointF ParseEasingInOut(const QJsonObject &object) {
	const auto x = object.value(QStringLiteral("x"));
	const auto y = object.value(QStringLiteral("y"));
	return x.isArray()
		? QPointF(x.toArray().at(0).toDouble(), y.toArray().at(0).toDouble())
		: QPointF(x.toDouble(), y.toDouble());
}

template <typename T>
T ParseValue(const QJsonArray &value) {
	const auto variant = value.at(0).toVariant();
	return variant.canConvert<T>() ? variant.value<T>() : T();
}

template <>
inline QPointF ParseValue<QPointF>(const QJsonArray &value) {
	return (value.count() > 1)
		? QPointF(value.at(0).toDouble(), value.at(1).toDouble())
		: QPointF();
}

template <>
inline QSizeF ParseValue<QSizeF>(const QJsonArray &value) {
	return (value.count() > 1)
		? QSizeF(value.at(0).toDouble(), value.at(1).toDouble())
		: QSizeF();
}

template <>
inline QVector4D ParseValue<QVector4D>(const QJsonArray &value) {
	return (value.count() > 3)
		? QVector4D(
			value.at(0).toDouble(),
			value.at(1).toDouble(),
			value.at(2).toDouble(),
			value.at(3).toDouble())
		: QVector4D();
}

template <typename T>
T ParseValue(const QJsonValue &value) {
	if (value.isArray()) {
		return ParseValue<T>(value.toArray());
	}
	const auto variant = value.toVariant();
	return variant.canConvert<T>() ? variant.value<T>() : T();
}

template <typename T>
ConstructKeyframeData<T> ParseKeyframe(const QJsonObject &keyframe) {
	auto result = ConstructKeyframeData<T>();

	result.startFrame = keyframe.value(QStringLiteral("t")).toVariant().toInt();
	result.startValue = ParseValue<T>(keyframe.value(QStringLiteral("s")).toArray());
	result.endValue = ParseValue<T>(keyframe.value(QStringLiteral("e")).toArray());
	result.hold = (keyframe.value(QStringLiteral("h")).toInt() == 1);

	const auto in = keyframe.value(QStringLiteral("i")).toObject();
	const auto out = keyframe.value(QStringLiteral("o")).toObject();
	result.easingIn = ParseEasingInOut(keyframe.value(QStringLiteral("i")).toObject());
	result.easingOut = ParseEasingInOut(keyframe.value(QStringLiteral("o")).toObject());

	if constexpr (std::is_same_v<T, QPointF>) {
		const auto tin = keyframe.value(QStringLiteral("ti")).toArray();
		const auto tout = keyframe.value(QStringLiteral("to")).toArray();
		result.tangentIn = QPointF(tin.at(0).toDouble(), tin.at(1).toDouble());
		result.tangentOut = QPointF(tout.at(0).toDouble(), tout.at(2).toDouble());
	}

	return result;
}

template <typename T>
ConstructAnimatedData<T> ParseAnimatedData(const QJsonArray &keyframes) {
	auto result = ConstructAnimatedData<T>();
	result.keyframes.reserve(keyframes.count());
	for (const auto &keyframe : keyframes) {
		result.keyframes.push_back(ParseKeyframe<T>(keyframe.toObject()));
	}
	return result;
}

template<typename T>
class BMProperty final {
public:
	void constructAnimated(const ConstructAnimatedData<T> &data) {
		m_animated = true;

		const auto &list = data.keyframes;
		m_easingCurves.reserve(list.size());
		const auto b = begin(list);
		const auto e = end(list);
		for (auto i = b; i != e; ++i) {
			const auto prev = (i == b) ? i : (i - 1);
			const auto next = i + 1;
			m_easingCurves.push_back(createEasing(
				(prev != i) ? &*prev : nullptr,
				*i,
				(next != e) ? &*next : nullptr));
		}
		if (!m_easingCurves.empty()) {
			m_startFrame = std::round(m_easingCurves.front().startFrame);
			m_endFrame = std::round(m_easingCurves.back().endFrame);
		}
	}

	void construct(const QJsonObject &definition) {
		if (definition.value(QStringLiteral("s")).toVariant().toInt()) {
			qWarning()
				<< "Property is split into separate x and y but it is not supported";
		}

		const auto value = definition.value(QStringLiteral("k"));
		const auto animated = value.isArray() && value.toArray().at(0).isObject();
		if (animated) {
			constructAnimated(ParseAnimatedData<T>(value.toArray()));
		} else {
			setValue(ParseValue<T>(value));
		}
	}

	void setValue(const T& value) {
		m_value = value;
	}

	const T& value() const {
		return m_value;
	}

	bool animated() const {
		return m_animated;
	}

	bool update(int frame) {
		if (!m_animated) {
			return false;
		}

		int adjustedFrame = qBound(m_startFrame, frame, m_endFrame);
		if (const EasingSegment<T> *easing = getEasingSegment(adjustedFrame)) {
			if (easing->easing.isHold()) {
				m_value = easing->startValue;
			} else if (easing->endFrame > easing->startFrame) {
				qreal progress = ((adjustedFrame - easing->startFrame) * 1.0)
					/ (easing->endFrame - easing->startFrame);
				qreal easedValue = easing->easing.valueForProgress(progress);
				if constexpr (std::is_same_v<QVector4D, T>) {
					// For the time being, 4D vectors are used only for colors, and
					// the value must be restricted to between [0, 1]
					easedValue = qBound(qreal(0.0), easedValue, qreal(1.0));
					m_value = easing->startValue + easedValue
						* ((easing->endValue - easing->startValue));
				} else if constexpr (std::is_same_v<QPointF, T>) {
					if (easing->bezierPoints.empty()) {
						m_value = easing->startValue + easedValue
							* ((easing->endValue - easing->startValue));
					} else {
						const auto distance = easedValue * easing->bezierLength;
						auto segmentPerc = 0.;
						auto addedLength = 0.;
						const auto count = easing->bezierPoints.size();
						for (auto j = 0; j != count; ++j) {
							addedLength += easing->bezierPoints[j].length;
							if (distance == 0. || easedValue == 0. || j == count - 1) {
								m_value = easing->bezierPoints[j].point;
								break;
							} else if (distance >= addedLength && distance < addedLength + easing->bezierPoints[j + 1].length) {
								segmentPerc = (distance - addedLength) / easing->bezierPoints[j + 1].length;
								m_value = easing->bezierPoints[j].point + (easing->bezierPoints[j + 1].point - easing->bezierPoints[j].point) * segmentPerc;
								break;
							}
						}
					}
				} else {
					m_value = easing->startValue + easedValue
						* ((easing->endValue - easing->startValue));
				}
			} else {
				m_value = easing->endValue;
			}
			return true;
		}
		return false;
	}

protected:
	const EasingSegment<T>* getEasingSegment(int frame) {
		// TODO: Improve with a faster search algorithm
		const EasingSegment<T> *easing = m_currentEasing;
		if (!easing
			|| easing->startFrame < frame
			|| easing->endFrame > frame) {
			for (int i=0; i < m_easingCurves.size(); i++) {
				if (m_easingCurves.at(i).startFrame <= frame
					&& m_easingCurves.at(i).endFrame >= frame) {
					m_currentEasing = &m_easingCurves.at(i);
					break;
				}
			}
		}

		if (!m_currentEasing) {
			qWarning()
				<< "Property is animated but easing cannot be found";
		}
		return m_currentEasing;
	}

	EasingSegment<T> createEasing(
			const ConstructKeyframeData<T> *prev,
			const ConstructKeyframeData<T> &data,
			const ConstructKeyframeData<T> *next) {
		auto result = EasingSegment<T>();
		result.startFrame = data.startFrame;
		result.endFrame = next ? next->startFrame : data.startFrame;
		result.startValue = (prev && prev->endValue != T())
			? prev->endValue
			: data.startValue;
		result.endValue = !next
			? data.startValue
			: (next->startValue != T())
			? next->startValue
			: data.endValue;
		if (data.hold) {
			result.easing.set(
				QPointF(0., 0.),
				QPointF(0.25, 0.),
				QPointF(0.75, 0.),
				QPointF(1., 0.));
			return result;
		}
		result.easing.set(
			QPointF(0., 0.),
			data.easingOut,
			data.easingIn,
			QPointF(1., 1.));
		if constexpr (std::is_same_v<QPointF, T>) {
			// TODO optimize case when they all are on one line.
			if (data.tangentIn == QPointF()
				&& data.tangentOut == QPointF()) {
				return result;
			}
			const auto bezier = QBezier::fromPoints(
				result.startValue,
				result.startValue + data.tangentOut,
				result.endValue + data.tangentIn,
				result.endValue);

			const auto kCount = 150;
			result.bezierPoints.reserve(kCount);
			for (auto k = 0; k < kCount; ++k) {
				const auto percent = double(k) / (kCount - 1.);
				auto point = EasingSegment<QPointF>::BezierPoint();
				point.point = bezier.pointAt(percent);
				if (k > 0) {
					const auto delta = (point.point - result.bezierPoints.back().point);
					point.length = std::sqrt(QPointF::dotProduct(delta, delta));
					result.bezierLength += point.length;
				}
				result.bezierPoints.push_back(point);
			}
		}
		return result;
	}

protected:
	bool m_animated = false;
	QVector<EasingSegment<T>> m_easingCurves;
	const EasingSegment<T> *m_currentEasing = nullptr;
	int m_startFrame = INT_MAX;
	int m_endFrame = 0;
	T m_value = T();

};

} // namespace Lottie
