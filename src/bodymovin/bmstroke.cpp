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
#include "bmstroke.h"

#include "renderer.h"

namespace Lottie {

BMStroke::BMStroke(BMBase *parent) : BMShape(parent) {
}

BMStroke::BMStroke(BMBase *parent, const BMStroke &other)
: BMShape(parent, other)
, m_opacity(other.m_opacity)
, m_width(other.m_width)
, m_color(other.m_color)
, m_capStyle(other.m_capStyle)
, m_joinStyle(other.m_joinStyle)
, m_miterLimit(other.m_miterLimit) {
}

BMStroke::BMStroke(BMBase *parent, const QJsonObject &definition)
: BMShape(parent) {
	BMBase::parse(definition);
	if (m_hidden) {
		return;
	}

	int lineCap = definition.value(QStringLiteral("lc")).toVariant().toInt();
	switch (lineCap) {
	case 1:
		m_capStyle = Qt::FlatCap;
		break;
	case 2:
		m_capStyle = Qt::RoundCap;
		break;
	case 3:
		m_capStyle = Qt::SquareCap;
		break;
	default:
		qWarning() << "Unknown line cap style in BMStroke";
	}

	int lineJoin = definition.value(QStringLiteral("lj")).toVariant().toInt();
	switch (lineJoin) {
	case 1:
		m_joinStyle = Qt::MiterJoin;
		m_miterLimit = definition.value(QStringLiteral("ml")).toVariant().toReal();
		break;
	case 2:
		m_joinStyle = Qt::RoundJoin;
		break;
	case 3:
		m_joinStyle = Qt::BevelJoin;
		break;
	default:
		qWarning() << "Unknown line join style in BMStroke";
	}

	const auto opacity = definition.value(QStringLiteral("o")).toObject();
	m_opacity.construct(opacity);

	const auto width = definition.value(QStringLiteral("w")).toObject();
	m_width.construct(width);

	const auto color = definition.value(QStringLiteral("c")).toObject();
	m_color.construct(color);
}

BMBase *BMStroke::clone(BMBase *parent) const {
	return new BMStroke(parent, *this);
}

void BMStroke::updateProperties(int frame) {
	m_opacity.update(frame);
	m_width.update(frame);
	m_color.update(frame);
}

void BMStroke::render(Renderer &renderer, int frame) const {
	renderer.render(*this);
}

QPen BMStroke::pen() const {
	qreal width = m_width.value();
	if (qFuzzyIsNull(width)) {
		return QPen(Qt::NoPen);
	}
	QPen pen;
	QColor color(getColor());
	color.setAlphaF(color.alphaF() * (opacity() / 100.));
	pen.setColor(color);
	pen.setWidthF(width);
	pen.setCapStyle(m_capStyle);
	pen.setJoinStyle(m_joinStyle);
	pen.setMiterLimit(m_miterLimit);
	return pen;
}

QColor BMStroke::getColor() const {
	QVector4D cVec = m_color.value();
	QColor color;
	qreal r = static_cast<qreal>(cVec.x());
	qreal g = static_cast<qreal>(cVec.y());
	qreal b = static_cast<qreal>(cVec.z());
	qreal a = static_cast<qreal>(cVec.w());
	color.setRgbF(r, g, b, a);
	return color;
}

qreal BMStroke::opacity() const {
	return m_opacity.value();
}

} // namespace Lottie
