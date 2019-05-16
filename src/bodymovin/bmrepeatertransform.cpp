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
#include "bmrepeatertransform.h"

#include "renderer.h"

namespace Lottie {

BMRepeaterTransform::BMRepeaterTransform(BMBase *parent) : BMBasicTransform(parent) {
}

BMRepeaterTransform::BMRepeaterTransform(BMBase *parent, const BMRepeaterTransform &other)
: BMBasicTransform(parent, other)
, m_startOpacity(other.m_startOpacity)
, m_endOpacity(other.m_endOpacity)
, m_opacities(other.m_opacities) {
}

BMRepeaterTransform::BMRepeaterTransform(BMBase *parent, const JsonObject &definition)
: BMBasicTransform(parent) {
	parse(definition);
}

BMBase *BMRepeaterTransform::clone(BMBase *parent) const {
	return new BMRepeaterTransform(parent, *this);
}

void BMRepeaterTransform::parse(const JsonObject &definition) {
	BMBasicTransform::parse(definition);
	if (m_hidden) {
		return;
	}

	const auto startOpacity = definition.value("so").toObject();
	m_startOpacity.construct(startOpacity);

	const auto endOpacity = definition.value("eo").toObject();
	m_endOpacity.construct(endOpacity);
}

void BMRepeaterTransform::updateProperties(int frame) {
	BMBasicTransform::updateProperties(frame);

	m_startOpacity.update(frame);
	m_endOpacity.update(frame);

	m_opacities.clear();
	for (int i = 0; i < m_copies; i++) {
		const auto opacity = m_startOpacity.value()
			+ (m_endOpacity.value() - m_startOpacity.value()) * i / m_copies;
		m_opacities.push_back(opacity);
	}
}

void BMRepeaterTransform::render(Renderer &renderer, int frame) const {
	renderer.render(*this);
}

void BMRepeaterTransform::setInstanceCount(int copies) {
	m_copies = copies;
}

qreal BMRepeaterTransform::opacityAtInstance(int instance) const {
	return m_opacities.at(instance) / 100.0;
}

qreal BMRepeaterTransform::startOpacity() const {
	return m_startOpacity.value();
}

qreal BMRepeaterTransform::endOpacity() const {
	return m_endOpacity.value();
}

} // namespace Lottie
