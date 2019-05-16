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
#include "bmrect.h"

#include "bmtrimpath.h"
#include "renderer.h"

#include <QJsonObject>
#include <QJsonArray>

namespace Lottie {

BMRect::BMRect(BMBase *parent) : BMShape(parent) {
}

BMRect::BMRect(BMBase *parent, const BMRect &other)
: BMShape(parent, other)
, m_position(other.m_position)
, m_size(other.m_size)
, m_roundness(other.m_roundness) {
}

BMRect::BMRect(BMBase *parent, const QJsonObject &definition)
: BMShape(parent) {
	BMBase::parse(definition);
	if (m_hidden) {
		return;
	}

	const auto position = definition.value(QStringLiteral("p")).toObject();
	m_position.construct(position);

	const auto size = definition.value(QStringLiteral("s")).toObject();
	m_size.construct(size);

	const auto roundness = definition.value(QStringLiteral("r")).toObject();
	m_roundness.construct(roundness);

	m_direction = definition.value(QStringLiteral("d")).toInt();
}


BMBase *BMRect::clone(BMBase *parent) const {
	return new BMRect(parent, *this);
}

void BMRect::updateProperties(int frame) {
	m_size.update(frame);
	m_position.update(frame);
	m_roundness.update(frame);

	// AE uses center of a shape as it's position,
	// in Qt a translation is needed
	const auto pos = QPointF(
		m_position.value().x() - m_size.value().width() / 2,
		m_position.value().y() - m_size.value().height() / 2);

	m_path = QPainterPath();
	m_path.addRoundedRect(
		QRectF(pos, m_size.value()),
		m_roundness.value(),
		m_roundness.value());

	if (m_direction) {
		m_path = m_path.toReversed();
	}
}

void BMRect::render(Renderer &renderer, int frame) const {
	renderer.render(*this);
}

bool BMRect::acceptsTrim() const {
	return true;
}

QPointF BMRect::position() const {
	return m_position.value();
}

QSizeF BMRect::size() const {
	return m_size.value();
}

qreal BMRect::roundness() const {
	return m_roundness.value();
}

} // namespace Lottie
