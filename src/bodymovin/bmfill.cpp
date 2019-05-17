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
#include "bmfill.h"

#include "renderer.h"

#include <QColor>

namespace Lottie {

BMFill::BMFill(BMBase *parent) : BMShape(parent) {
}

BMFill::BMFill(BMBase *parent, const BMFill &other)
: BMShape(parent, other)
, m_color(other.m_color)
, m_opacity(other.m_opacity) {
}

BMFill::BMFill(BMBase *parent, const JsonObject &definition)
: BMShape(parent) {
	BMBase::parse(definition);
	if (m_hidden) {
		return;
	}

	const auto color = definition.value("c").toObject();
	m_color.construct(color);

	const auto opacity = definition.value("o").toObject();
	m_opacity.construct(opacity);
}

BMBase *BMFill::clone(BMBase *parent) const {
	return new BMFill(parent, *this);
}

void BMFill::updateProperties(int frame) {
	m_color.update(frame);
	m_opacity.update(frame);
}

void BMFill::render(Renderer &renderer, int frame) const {
	renderer.render(*this);
}

QColor BMFill::color() const {
	return ColorFromVector(m_color.value());
}

qreal BMFill::opacity() const {
	return m_opacity.value();
}

} // namespace Lottie
