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
#include <QJsonObject>

namespace Lottie {

QPainterPath FreeFormShape::parse(const QJsonObject &definition) {
	if (!definition.value(QStringLiteral("a")).toInt()) {
		return buildShape(definition.value(QStringLiteral("k")).toObject());
	}
	parseShapeKeyframes(definition);
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

void FreeFormShape::parseShapeKeyframes(const QJsonObject &keyframes) {
	struct Entry {
		ConstructAnimatedData<QPointF> pos;
		ConstructAnimatedData<QPointF> ci;
		ConstructAnimatedData<QPointF> co;
	};
	auto entries = std::vector<Entry>();

	QJsonArray vertexKeyframes = keyframes.value(QStringLiteral("k")).toArray();
	for (int i = 0; i < vertexKeyframes.count(); i++) {
		QJsonObject keyframe = vertexKeyframes.at(i).toObject();

		const auto hold = (keyframe.value(QStringLiteral("h")).toInt() == 1);
		const auto startFrame = keyframe.value(QStringLiteral("t")).toVariant().toInt();

		QJsonObject startValue = keyframe.value(QStringLiteral("s")).toArray().at(0).toObject();
		QJsonObject endValue = keyframe.value(QStringLiteral("e")).toArray().at(0).toObject();
		bool closedPathAtStart = keyframe.value(QStringLiteral("s")).toArray().at(0).toObject().value(QStringLiteral("c")).toBool();
		//bool closedPathAtEnd = keyframe.value(QStringLiteral("e")).toArray().at(0).toObject().value(QStringLiteral("c")).toBool();
		QJsonArray startVertices = startValue.value(QStringLiteral("v")).toArray();
		QJsonArray startBezierIn = startValue.value(QStringLiteral("i")).toArray();
		QJsonArray startBezierOut = startValue.value(QStringLiteral("o")).toArray();
		QJsonArray endVertices = endValue.value(QStringLiteral("v")).toArray();
		QJsonArray endBezierIn = endValue.value(QStringLiteral("i")).toArray();
		QJsonArray endBezierOut = endValue.value(QStringLiteral("o")).toArray();
		QJsonObject easingIn = keyframe.value(QStringLiteral("i")).toObject();
		QJsonObject easingOut = keyframe.value(QStringLiteral("o")).toObject();

		if (!startVertices.isEmpty()
			&& !entries.empty()
			&& startVertices.size() != entries.size()) {
			qWarning() << "Bad data in shape.";
			return;
		}
		const auto count = startVertices.isEmpty()
			? int(entries.size())
			: int(startVertices.size());
		if (!count) {
			qWarning() << "Bad data in shape.";
			return;
		}
		if (entries.empty()) {
			entries.resize(count);
		}
		for (auto i = 0; i != count; ++i) {
			auto pos = ConstructKeyframeData<QPointF>();
			auto ci = ConstructKeyframeData<QPointF>();
			auto co = ConstructKeyframeData<QPointF>();

			pos.hold = ci.hold = co.hold = hold;
			pos.startFrame = ci.startFrame = co.startFrame = startFrame;

			if (!startVertices.isEmpty()) {
				pos.easingIn = ci.easingIn = co.easingIn = ParseEasingInOut(easingIn);
				pos.easingOut = ci.easingOut = co.easingOut = ParseEasingInOut(easingOut);

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
			!startVertices.isEmpty() && closedPathAtStart);
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

QPainterPath FreeFormShape::buildShape(const QJsonObject &shape) {
	auto result = QPainterPath();

	bool needToClose = shape.value(QStringLiteral("c")).toBool();
	QJsonArray bezierIn = shape.value(QStringLiteral("i")).toArray();
	QJsonArray bezierOut = shape.value(QStringLiteral("o")).toArray();
	QJsonArray vertices = shape.value(QStringLiteral("v")).toArray();

	// If there are less than two vertices, cannot make a bezier curve
	if (vertices.count() < 2) {
		return result;
	}

	QPointF s(
		vertices.at(0).toArray().at(0).toDouble(),
		vertices.at(0).toArray().at(1).toDouble());
	QPointF s0(s);

	result.moveTo(s);
	int i=0;

	while (i < vertices.count() - 1) {
		QPointF v = QPointF(
			vertices.at(i + 1).toArray().at(0).toDouble(),
			vertices.at(i + 1).toArray().at(1).toDouble());
		QPointF c1 = QPointF(
			bezierOut.at(i).toArray().at(0).toDouble(),
			bezierOut.at(i).toArray().at(1).toDouble());
		QPointF c2 = QPointF(
			bezierIn.at(i + 1).toArray().at(0).toDouble(),
			bezierIn.at(i + 1).toArray().at(1).toDouble());
		c1 += s;
		c2 += v;

		result.cubicTo(c1, c2, v);

		s = v;
		i++;
	}

	if (needToClose) {
		QPointF v = s0;
		QPointF c1 = QPointF(
			bezierOut.at(i).toArray().at(0).toDouble(),
			bezierOut.at(i).toArray().at(1).toDouble());
		QPointF c2 = QPointF(
			bezierIn.at(0).toArray().at(0).toDouble(),
			bezierIn.at(0).toArray().at(1).toDouble());
		c1 += s;
		c2 += v;

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
