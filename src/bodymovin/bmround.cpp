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
#include "bmround.h"

#include "bmtrimpath.h"
#include "renderer.h"

#include <QJsonObject>

namespace Lottie {

BMRound::BMRound(BMBase *parent) : BMShape(parent) {
}

BMRound::BMRound(BMBase *parent, const BMRound &other)
: BMShape(parent, other)
, m_position(other.m_position)
, m_radius(other.m_radius) {
}

BMRound::BMRound(BMBase *parent, const QJsonObject &definition)
: BMShape(parent) {
	parse(definition);
}

BMBase *BMRound::clone(BMBase *parent) const {
	return new BMRound(parent, *this);
}

void BMRound::parse(const QJsonObject &definition) {
	BMBase::parse(definition);
	if (m_hidden) {
		return;
	}

	const auto position = definition.value(QStringLiteral("p")).toObject();
	m_position.construct(position);

	const auto radius = definition.value(QStringLiteral("r")).toObject();
	m_radius.construct(radius);
}

void BMRound::updateProperties(int frame) {
	m_position.update(frame);
	m_radius.update(frame);

	// AE uses center of a shape as it's position,
	// in Qt a translation is needed
	const auto center = QPointF(
		m_position.value().x() - m_radius.value() / 2,
		m_position.value().y() - m_radius.value() / 2);

	m_path = QPainterPath();
	m_path.arcMoveTo(
		QRectF(center, QSizeF(m_radius.value(), m_radius.value())), 90);
	m_path.arcTo(
		QRectF(center, QSizeF(m_radius.value(), m_radius.value())), 90, -360);

	if (m_direction) {
		m_path = m_path.toReversed();
	}
}

void BMRound::render(Renderer &renderer, int frame) const {
	renderer.render(*this);
}

bool BMRound::acceptsTrim() const {
	return true;
}

QPointF BMRound::position() const {
	return m_position.value();
}

qreal BMRound::radius() const {
	return m_radius.value();
}

} // namespace Lottie
