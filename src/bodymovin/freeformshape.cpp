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
#include "bmfreeformshape.h"

#include <QPainterPath>

namespace Lottie {

QPainterPath FreeFormShape::parse(const JsonObject &definition) {
	const auto value = definition.value("k");
	const auto animated = value.isArray();
	if (!animated) {
		return buildShape(value.toObject());
	}
	parseShapeKeyframes(value.toArray());
	return QPainterPath();
}

QPainterPath FreeFormShape::build(int frame) {
	for (auto &info : m_vertexList) {
		info.pos.update(frame);
		info.ci.update(frame);
		info.co.update(frame);
	}
	return buildShape(frame);
}

void FreeFormShape::parseShapeKeyframes(const JsonArray &keyframes) {
	struct Entry {
		ConstructAnimatedData<QPointF> pos;
		ConstructAnimatedData<QPointF> ci;
		ConstructAnimatedData<QPointF> co;
	};
	auto entries = std::vector<Entry>();

	for (const auto &element : keyframes) {
		const auto keyframe = element.toObject();

		const auto hold = (keyframe.value("h").toInt() == 1);
		const auto startFrame = keyframe.value("t").toInt();
		const auto easingIn = ParseEasingInOut(keyframe.value("i").toObject());
		const auto easingOut = ParseEasingInOut(keyframe.value("o").toObject());

		const auto startValue = keyframe.value("s").toArray().at(0).toObject();
		const auto endValue = keyframe.value("e").toArray().at(0).toObject();
		const auto closedPathAtStart = keyframe.value("s").toArray().at(0).toObject().value("c").toBool();
		//const auto closedPathAtEnd = keyframe.value("e").toArray().at(0).toObject().value("c").toBool();
		const auto startVertices = startValue.value("v").toArray();
		const auto startBezierIn = startValue.value("i").toArray();
		const auto startBezierOut = startValue.value("o").toArray();
		const auto endVertices = endValue.value("v").toArray();
		const auto endBezierIn = endValue.value("i").toArray();
		const auto endBezierOut = endValue.value("o").toArray();

		if (!startVertices.empty()
			&& !entries.empty()
			&& startVertices.size() != entries.size()) {
			qWarning() << "Bad data in shape.";
			return;
		}
		const auto count = startVertices.empty()
			? int(entries.size())
			: int(startVertices.size());
		if (!count) {
			qWarning() << "Bad data in shape.";
			return;
		}
		if (entries.empty()) {
			entries.resize(count);
			for (auto &entry : entries) {
				entry.pos.keyframes.reserve(keyframes.size());
				entry.ci.keyframes.reserve(keyframes.size());
				entry.co.keyframes.reserve(keyframes.size());
			}
		}
		for (auto i = 0; i != count; ++i) {
			auto pos = ConstructKeyframeData<QPointF>();
			auto ci = ConstructKeyframeData<QPointF>();
			auto co = ConstructKeyframeData<QPointF>();

			pos.hold = ci.hold = co.hold = hold;
			pos.startFrame = ci.startFrame = co.startFrame = startFrame;

			if (!startVertices.empty()) {
				pos.easingIn = ci.easingIn = co.easingIn = easingIn;
				pos.easingOut = ci.easingOut = co.easingOut = easingOut;

				pos.startValue = ParseValue<QPointF>(startVertices.at(i).toArray());
				pos.endValue = ParseValue<QPointF>(endVertices.at(i).toArray());

				ci.startValue = ParseValue<QPointF>(startBezierIn.at(i).toArray());
				ci.endValue = ParseValue<QPointF>(endBezierIn.at(i).toArray());

				co.startValue = ParseValue<QPointF>(startBezierOut.at(i).toArray());
				co.endValue = ParseValue<QPointF>(endBezierOut.at(i).toArray());
			}

			entries[i].pos.keyframes.push_back(std::move(pos));
			entries[i].ci.keyframes.push_back(std::move(ci));
			entries[i].co.keyframes.push_back(std::move(co));
		}
		m_closedShape.insert(
			startFrame,
			!startVertices.empty() && closedPathAtStart);
	}

	if (entries.empty()) {
		return;
	}
	m_vertexList.reserve(entries.size());
	for (const auto &entry : entries) {
		m_vertexList.push_back({});
		auto &info = m_vertexList.back();
		info.pos.constructAnimated(entry.pos);
		info.ci.constructAnimated(entry.ci);
		info.co.constructAnimated(entry.co);
	}
}

QPainterPath FreeFormShape::buildShape(const JsonObject &shape) {
	auto result = QPainterPath();

	const auto needToClose = shape.value("c").toBool();
	const auto bezierIn = shape.value("i").toArray();
	const auto bezierOut = shape.value("o").toArray();
	const auto vertices = shape.value("v").toArray();

	// If there are less than two vertices, cannot make a bezier curve
	if (vertices.size() < 2) {
		return result;
	}

	auto s = QPointF(
		vertices.at(0).toArray().at(0).toDouble(),
		vertices.at(0).toArray().at(1).toDouble());
	const auto s0 = s;

	result.moveTo(s);
	int i=0;

	while (i < vertices.size() - 1) {
		const auto v = QPointF(
			vertices.at(i + 1).toArray().at(0).toDouble(),
			vertices.at(i + 1).toArray().at(1).toDouble());
		const auto c1 = s + QPointF(
			bezierOut.at(i).toArray().at(0).toDouble(),
			bezierOut.at(i).toArray().at(1).toDouble());
		const auto c2 = v + QPointF(
			bezierIn.at(i + 1).toArray().at(0).toDouble(),
			bezierIn.at(i + 1).toArray().at(1).toDouble());

		result.cubicTo(c1, c2, v);

		s = v;
		i++;
	}

	if (needToClose) {
		const auto v = s0;
		const auto c1 = s + QPointF(
			bezierOut.at(i).toArray().at(0).toDouble(),
			bezierOut.at(i).toArray().at(1).toDouble());
		const auto c2 = v + QPointF(
			bezierIn.at(0).toArray().at(0).toDouble(),
			bezierIn.at(0).toArray().at(1).toDouble());
		result.cubicTo(c1, c2, v);
	}

	result.setFillRule(Qt::WindingFill);
	return result;
}

QPainterPath FreeFormShape::buildShape(int frame) {
	auto result = QPainterPath();

	auto it = m_closedShape.constBegin();
	bool found = false;

	if (frame <= it.key()) {
		found = true;
	} else {
		while (it != m_closedShape.constEnd()) {
			if (it.key() <= frame) {
				found = true;
				break;
			}
			++it;
		}
	}

	bool needToClose = false;
	if (found) {
		needToClose = (*it);
	}

	// If there are less than two vertices, cannot make a bezier curve.
	if (m_vertexList.size() < 2) {
		return result;
	}

	QPointF s(m_vertexList.at(0).pos.value());
	QPointF s0(s);

	result.moveTo(s);
	int i = 0;

	while (i < m_vertexList.size() - 1) {
		QPointF v = m_vertexList.at(i + 1).pos.value();
		QPointF c1 = m_vertexList.at(i).co.value();
		QPointF c2 = m_vertexList.at(i + 1).ci.value();
		c1 += s;
		c2 += v;

		result.cubicTo(c1, c2, v);

		s = v;
		i++;
	}

	if (needToClose) {
		QPointF v = s0;
		QPointF c1 = m_vertexList.at(i).co.value();
		QPointF c2 = m_vertexList.at(0).ci.value();
		c1 += s;
		c2 += v;

		result.cubicTo(c1, c2, v);
	}

	result.setFillRule(Qt::WindingFill);
	return result;
}

} // namespace Lottie
