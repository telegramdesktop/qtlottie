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
#include "bmmaskshape.h"

#include "bmtrimpath.h"
#include "renderer.h"

#include <QJsonObject>

namespace Lottie {

BMMaskShape::BMMaskShape(BMBase *parent) : BMShape(parent) {
}

BMMaskShape::BMMaskShape(BMBase *parent, const BMMaskShape &other)
: BMShape(parent, other)
, m_shape(other.m_shape) {
}

BMMaskShape::BMMaskShape(BMBase *parent, const QJsonObject &definition)
: BMShape(parent) {
	parse(definition);
}

BMBase *BMMaskShape::clone(BMBase *parent) const {
	return new BMMaskShape(parent, *this);
}

void BMMaskShape::parse(const QJsonObject &definition) {
	BMBase::parse(definition);

	m_inverted = definition.value(QStringLiteral("inv")).toBool();

	const auto opacity = definition.value(QStringLiteral("o")).toObject();
	if (opacity.isEmpty()) {
		m_opacity.setValue(100.);
	} else {
		m_opacity.construct(opacity);
	}

	if (m_opacity.value() < 100. || m_opacity.animated()) {
		qWarning() << "Transparent mask shapes are not supported.";
	}

	const auto mode = definition.value(QStringLiteral("mode")).toVariant().toString();
	if (mode == QStringLiteral("a")) {
		m_mode = Mode::Additive;
	} else if (mode == QStringLiteral("i")) {
		m_mode = Mode::Intersect;
	} else {
		qWarning() << "Unsupported mask mode.";
	}

	m_path = m_shape.parse(definition.value(QStringLiteral("pt")).toObject());
}

void BMMaskShape::updateProperties(int frame) {
	m_opacity.update(frame);
	if (m_path.isEmpty()) {
		m_path = m_shape.build(frame);
	}
}

void BMMaskShape::render(Renderer &renderer, int frame) const {
	renderer.render(*this);
}

BMMaskShape::Mode BMMaskShape::mode() const {
	return m_mode;
}

bool BMMaskShape::inverted() const {
	return m_inverted;
}

} // namespace Lottie
