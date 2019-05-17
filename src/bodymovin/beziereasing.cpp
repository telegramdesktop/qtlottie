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
#include "beziereasing.h"

namespace Lottie {

void BezierEasing::set(
		const QPointF &startPoint,
		const QPointF &c1,
		const QPointF &c2,
		const QPointF &endPoint) {
	mBezier = QBezier::fromPoints(startPoint, c1, c2, endPoint);
}

bool BezierEasing::isHold() const {
	return (mBezier.y4 == 0.);
}

bool BezierEasing::isLinear() const {
	return (mBezier.x2 == mBezier.y2) && (mBezier.x3 == mBezier.y3);
}

qreal BezierEasing::valueForProgress(qreal progress) const {
	return isLinear()
		? progress
		: std::clamp(mBezier.pointAt(tForX(progress)).y(), 0., 1.);
}

qreal BezierEasing::tForX(qreal x) const {
	if (x <= 0.0) {
		return 0.0;
	} else if (x >= 1.0) {
		return 1.0;
	}

	qreal t0 = 0.0;
	qreal t1 = 1.0;

	// 10 iterations give an error smaller than 0.001
	for (int i = 0; i < 10; i++) {
		qreal t = qreal(0.5) * (t0 + t1);
		qreal a, b, c, d;
		QBezier::coefficients(t, a, b, c, d);
		qreal xt = a * mBezier.x1
			+ b * mBezier.x2
			+ c * mBezier.x3
			+ d * mBezier.x4;
		if (xt < x) {
			t0 = t;
		} else {
			t1 = t;
		}
	}

	return t0;
}

} // namespace Lottie
