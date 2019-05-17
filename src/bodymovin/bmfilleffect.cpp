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
#include "bmfilleffect.h"

#include "renderer.h"

#include <QColor>

namespace Lottie {

BMFillEffect::BMFillEffect(BMBase *parent) : BMBase(parent) {
}

BMFillEffect::BMFillEffect(BMBase *parent, const BMFillEffect &other)
: BMBase(parent, other)
, m_color(other.m_color)
, m_opacity(other.m_opacity) {
}

BMFillEffect::BMFillEffect(BMBase *parent, const JsonObject &definition)
: BMBase(parent) {
	parse(definition);
}

BMBase *BMFillEffect::clone(BMBase *parent) const {
	return new BMFillEffect(parent, *this);
}

void BMFillEffect::parse(const JsonObject &definition) {
	m_type = BM_EFFECT_FILL;

	if (!definition.value("hd").toBool(true)) {
		return;
	}

	const auto properties = definition.value("ef").toArray();

	// TODO: Check are property positions really fixed in the effect?

	m_color.construct(properties.at(2).toObject().value("v").toObject());
	m_opacity.construct(properties.at(6).toObject().value("v").toObject());

	if (!qFuzzyCompare(properties.at(0).toObject().value("v").toObject().value("k").toDouble(), 0.0)) {
		qWarning() << "BMFillEffect: Property 'Fill mask' not supported";
	}

	if (!qFuzzyCompare(properties.at(1).toObject().value("v").toObject().value("k").toDouble(), 0.0)) {
		qWarning() << "BMFillEffect: Property 'All masks' not supported";
	}

	if (!qFuzzyCompare(properties.at(3).toObject().value("v").toObject().value("k").toDouble(), 0.0)) {
		qWarning() << "BMFillEffect: Property 'Invert' not supported";
	}

	if (!qFuzzyCompare(properties.at(4).toObject().value("v").toObject().value("k").toDouble(), 0.0)) {
		qWarning() << "BMFillEffect: Property 'Horizontal feather' not supported";
	}

	if (!qFuzzyCompare(properties.at(5).toObject().value("v").toObject().value("k").toDouble(), 0.0)) {
		qWarning() << "BMFillEffect: Property 'Vertical feather' not supported";
	}
}

void BMFillEffect::updateProperties(int frame) {
	m_color.update(frame);
	m_opacity.update(frame);
}

void BMFillEffect::render(Renderer &renderer, int frame) const {
	renderer.render(*this);
}

QColor BMFillEffect::color() const {
	return ColorFromVector(m_color.value());
}

qreal BMFillEffect::opacity() const {
	return m_opacity.value();
}

} // namespace Lottie
