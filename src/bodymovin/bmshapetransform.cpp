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
#include "bmshapetransform.h"

#include "bmbasictransform.h"
#include "renderer.h"

#include <QtMath>

namespace Lottie {

BMShapeTransform::BMShapeTransform(BMBase *parent, const BMShapeTransform &other)
: BMBasicTransform(parent, other)
, m_skew(other.m_skew)
, m_skewAxis(other.m_skewAxis)
, m_shearX(other.m_shearX)
, m_shearY(other.m_shearY)
, m_shearAngle(other.m_shearAngle) {
}

BMShapeTransform::BMShapeTransform(BMBase *parent, const JsonObject &definition)
: BMBasicTransform(parent) {
	parse(definition);
}

BMBase *BMShapeTransform::clone(BMBase *parent) const {
	return new BMShapeTransform(parent, *this);
}

void BMShapeTransform::parse(const JsonObject &definition) {
	BMBasicTransform::parse(definition);

	const auto skew = definition.value("sk").toObject();
	m_skew.construct(skew);

	const auto skewAxis = definition.value("sa").toObject();
	m_skewAxis.construct(skewAxis);
}

void BMShapeTransform::updateProperties(int frame) {
	BMBasicTransform::updateProperties(frame);

	m_skew.update(frame);
	m_skewAxis.update(frame);

	const auto rads = qDegreesToRadians(m_skewAxis.value());
	m_shearX = qCos(rads);
	m_shearY = qSin(rads);
	const auto tan = qDegreesToRadians(-m_skew.value());
	m_shearAngle = qTan(tan);
}

void BMShapeTransform::render(Renderer &renderer, int frame) const {
	renderer.render(*this);
}

qreal BMShapeTransform::skew() const {
	return m_skew.value();
}

qreal BMShapeTransform::skewAxis() const {
	return m_skewAxis.value();
}

qreal BMShapeTransform::shearX() const {
	return m_shearX;
}

qreal BMShapeTransform::shearY() const {
	return m_shearY;
}

qreal BMShapeTransform::shearAngle() const {
	return m_shearAngle;
}

QTransform BMShapeTransform::apply(QTransform to) const {
	const auto pos = position();
	const auto rot = rotation();
	const auto sca = scale();
	const auto anc = anchorPoint();

	to.translate(pos.x(), pos.y());
	if (!qFuzzyIsNull(rot)) {
		to.rotate(rot);
	}
	if (!qFuzzyIsNull(skew())) {
		const auto shX = shearX();
		const auto shY = shearY();
		const auto ang = shearAngle();
		QTransform t(shX, shY, 0, -shY, shX, 0, 0, 0, 1);
		t *= QTransform(1, 0, 0, ang, 1, 0, 0, 0, 1);
		t *= QTransform(shX, -shY, 0, shY, shX, 0, 0, 0, 1);
		to = t * to;
	}
	to.scale(sca.x(), sca.y());
	to.translate(-anc.x(), -anc.y());

	return to;
}

} // namespace Lottie
