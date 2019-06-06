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
#include "json.h"

#include <QPointF>
#include <QSizeF>
#include <QVector4D>
#include <QColor>
#include <QtMath>
#include <vector>

namespace Lottie {

inline QColor ColorFromVector(const QVector4D &value) {
	return QColor::fromRgbF(
		std::clamp(double(value.x()), 0., 1.),
		std::clamp(double(value.y()), 0., 1.),
		std::clamp(double(value.z()), 0., 1.),
		std::clamp(double(value.w()), 0., 1.));
}

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
	mutable double bezierCacheDistance = 0.;
	mutable int bezierCacheIndex = 0;
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

inline QPointF ParseEasingInOut(const JsonObject &object) {
	const auto x = object.value("x");
	const auto y = object.value("y");
	return x.isArray()
		? QPointF(x.toArray().at(0).toDouble(), y.toArray().at(0).toDouble())
		: QPointF(x.toDouble(), y.toDouble());
}

template <typename T>
T ParsePlainValue(const JsonValue &value) {
	return T();
}

template <>
inline qreal ParsePlainValue<qreal>(const JsonValue &value) {
	return value.toDouble();
}

template <>
inline int ParsePlainValue<int>(const JsonValue &value) {
	return value.toInt();
}

template <typename T>
T ParseValue(const JsonArray &value) {
	return ParsePlainValue<T>(value.at(0));
}

template <>
inline QPointF ParseValue<QPointF>(const JsonArray &value) {
	return (value.size() > 1)
		? QPointF(value.at(0).toDouble(), value.at(1).toDouble())
		: QPointF();
}

template <>
inline QSizeF ParseValue<QSizeF>(const JsonArray &value) {
	return (value.size() > 1)
		? QSizeF(value.at(0).toDouble(), value.at(1).toDouble())
		: QSizeF();
}

template <>
inline QVector4D ParseValue<QVector4D>(const JsonArray &value) {
	return (value.size() > 3)
		? QVector4D(
			value.at(0).toDouble(),
			value.at(1).toDouble(),
			value.at(2).toDouble(),
			value.at(3).toDouble())
		: QVector4D();
}

template <typename T>
T ParseValue(const JsonValue &value) {
	return value.isArray()
		? ParseValue<T>(value.toArray())
		: ParsePlainValue<T>(value);
}

template <typename T>
ConstructKeyframeData<T> ParseKeyframe(const JsonObject &keyframe) {
	auto result = ConstructKeyframeData<T>();

	result.startFrame = keyframe.value("t").toInt();
	result.startValue = ParseValue<T>(keyframe.value("s").toArray());
	result.endValue = ParseValue<T>(keyframe.value("e").toArray());
	result.hold = (keyframe.value("h").toInt() == 1);

	const auto in = keyframe.value("i").toObject();
	const auto out = keyframe.value("o").toObject();
	result.easingIn = ParseEasingInOut(keyframe.value("i").toObject());
	result.easingOut = ParseEasingInOut(keyframe.value("o").toObject());

	if constexpr (std::is_same_v<T, QPointF>) {
		const auto tin = keyframe.value("ti").toArray();
		const auto tout = keyframe.value("to").toArray();
		result.tangentIn = QPointF(tin.at(0).toDouble(), tin.at(1).toDouble());
		result.tangentOut = QPointF(tout.at(0).toDouble(), tout.at(2).toDouble());
	}

	return result;
}

template <typename T>
ConstructAnimatedData<T> ParseAnimatedData(const JsonArray &keyframes) {
	auto result = ConstructAnimatedData<T>();
	result.keyframes.reserve(keyframes.size());
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

	void construct(const JsonObject &definition) {
		if (definition.value("s").toInt()) {
			qWarning()
				<< "Property is split into separate x and y but it is not supported.";
		}
		if (definition.contains("x")) {
			qWarning()
				<< "Expressions are not supported.";
		}
		const auto value = definition.value("k");
		const auto animated = value.isArray() && value.toArray().at(0).isObject();
		if (animated) {
			constructAnimated(ParseAnimatedData<T>(value.toArray()));
		} else {
			setValue(ParseValue<T>(value));
		}
	}

	void setValue(const T &value) {
		m_value = value;
	}

	const T &value() const {
		return m_value;
	}

	bool animated() const {
		return m_animated;
	}

	bool update(int frame) {
		if (!m_animated) {
			return false;
		}

		frame = std::clamp(frame, m_startFrame, m_endFrame);
		const auto easing = getEasingSegment(frame);
		if (!easing) {
			return false;
		}
		m_value = getEasingValue(*easing, frame);
		return true;
	}

private:
	const EasingSegment<T> *getEasingSegment(int frame) const {
		const auto count = m_easingCurves.size();
		//if (m_easingIndex >= count) {
		//	qWarning()
		//		<< "Property is animated but easing cannot be found";
		//	return nullptr;
		//}
		const auto from = /*(m_easingCurves[m_easingIndex].startFrame <= frame)
			? m_easingIndex
			: */0;
		for (auto i = from; i != count; ++i) {
			const auto &segment = m_easingCurves[i];
			if (segment.startFrame <= frame && segment.endFrame >= frame) {
				//m_easingIndex = i;
				return &segment;
			}
		}
		qWarning()
			<< "Property is animated but easing cannot be found";
		return nullptr;
	}

	T getEasingValue(const EasingSegment<T> &segment, int frame) const {
		if (segment.easing.isHold()) {
			return segment.startValue;
		} else if (segment.endFrame <= segment.startFrame) {
			return segment.endValue;
		}
		const auto progress = ((frame - segment.startFrame) * 1.0)
			/ (segment.endFrame - segment.startFrame);
		const auto percent = segment.easing.valueForProgress(progress);
		if constexpr (std::is_same_v<QPointF, T>) {
			if (!segment.bezierPoints.empty()) {
				return getSpatialValue(segment, percent);
			}
		}
		return segment.startValue
			+ percent * (segment.endValue - segment.startValue);
	}

	T getSpatialValue(const EasingSegment<T> &segment, double value) const {
		const auto count = segment.bezierPoints.size();
		if (segment.bezierCacheIndex >= count) {
			return segment.endValue;
		}
		const auto distance = value * segment.bezierLength;
		const auto from = (segment.bezierCacheDistance <= distance)
			? segment.bezierCacheIndex
			: 0;
		auto length = (segment.bezierCacheDistance <= distance)
			? segment.bezierCacheDistance
			: 0.;
		for (auto i = from; i != count; ++i) {
			const auto &point = segment.bezierPoints[i];
			if (distance == 0. || i == count - 1) {
				segment.bezierCacheDistance = length;
				segment.bezierCacheIndex = i;
				return point.point;
			}
			const auto &next = segment.bezierPoints[i + 1];
			if (distance >= length && distance < length + next.length) {
				segment.bezierCacheDistance = length;
				segment.bezierCacheIndex = i;
				const auto percent = (distance - length) / next.length;
				return point.point + percent * (next.point - point.point);
			}
			length += next.length;
		}
		return segment.endValue;
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
			const auto kPointOnLine = [](QPointF a, QPointF b, QPointF c) {
				return (a == b) || (b == c) || qFuzzyCompare(
					(a.x() * b.y()) + (a.y() * c.x()) + (b.x() * c.y()),
					(c.x() * b.y()) + (c.y() * a.x()) + (b.x() * a.y()));
			};
			const auto linear = kPointOnLine(
				QPointF(),
				data.tangentOut,
				result.endValue - result.startValue
			) && kPointOnLine(
				result.startValue - result.endValue,
				data.tangentIn,
				QPointF()
			);
			if (linear) {
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
	// caching doesn't work, we clone easing each time.
	//mutable int m_easingIndex = 0;
	int m_startFrame = INT_MAX;
	int m_endFrame = 0;
	T m_value = T();

};

} // namespace Lottie
