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
#include "bmshape.h"

#include "bmgroup.h"
#include "bmfill.h"
#include "bmgfill.h"
#include "bmstroke.h"
#include "bmrect.h"
#include "bmellipse.h"
#include "bmround.h"
#include "bmtrimpath.h"
#include "bmshapetransform.h"
#include "bmfreeformshape.h"
#include "bmrepeater.h"

namespace Lottie {

BMShape::BMShape(BMBase *parent) : BMBase(parent) {
}

BMShape::BMShape(BMBase *parent, const BMShape &other)
: BMBase(parent, other)
, m_path(other.m_path)
, m_appliedTrim(other.m_appliedTrim)
, m_direction(other.m_direction) {
}

BMBase *BMShape::clone(BMBase *parent) const {
    return new BMShape(parent, *this);
}

BMShape *BMShape::construct(BMBase *parent, const JsonObject &definition) {
    BMShape *shape = nullptr;
    const auto type = definition.value("ty").toString();

    if (Q_UNLIKELY(type.size() != 2)) {
		qWarning() << "Unsupported shape type:" << type;
        return shape;
    }

#define BM_SHAPE_TAG(c1, c2) int((quint32(c1)<<8) | quint32(c2))

    int typeToBuild = BM_SHAPE_TAG(type[0], type[1]);

    switch (typeToBuild) {
    case BM_SHAPE_TAG('g', 'r'):
        shape = new BMGroup(parent, definition);
        shape->setType(BM_SHAPE_GROUP_IX);
        break;

	case BM_SHAPE_TAG('r', 'c'):
        shape = new BMRect(parent, definition);
        shape->setType(BM_SHAPE_RECT_IX);
        break;

	case BM_SHAPE_TAG('f', 'l'):
        shape = new BMFill(parent, definition);
        shape->setType(BM_SHAPE_FILL_IX);
        break;

	case BM_SHAPE_TAG('g', 'f'):
        shape = new BMGFill(parent, definition);
        shape->setType(BM_SHAPE_GFILL_IX);
        break;

	case BM_SHAPE_TAG('s', 't'):
        shape = new BMStroke(parent, definition);
        shape->setType(BM_SHAPE_STROKE_IX);
        break;

	case BM_SHAPE_TAG('t', 'r'):
        shape = new BMShapeTransform(parent, definition);
        shape->setType(BM_SHAPE_TRANS_IX);
        break;

	case BM_SHAPE_TAG('e', 'l'):
        shape = new BMEllipse(parent, definition);
        shape->setType(BM_SHAPE_ELLIPSE_IX);
        break;

	case BM_SHAPE_TAG('r', 'd'):
        shape = new BMRound(parent, definition);
        shape->setType(BM_SHAPE_ROUND_IX);
        break;

	case BM_SHAPE_TAG('s', 'h'):
        shape = new BMFreeFormShape(parent, definition);
        shape->setType(BM_SHAPE_SHAPE_IX);
        break;

	case BM_SHAPE_TAG('t', 'm'):
        shape = new BMTrimPath(parent, definition);
        shape->setType(BM_SHAPE_TRIM_IX);
        break;

	case BM_SHAPE_TAG('r', 'p'):
        shape = new BMRepeater(parent, definition);
        shape->setType(BM_SHAPE_REPEATER_IX);
        break;

	case BM_SHAPE_TAG('g', 's'): // ### BM_SHAPE_GSTROKE_IX
    case BM_SHAPE_TAG('s', 'r'): // ### BM_SHAPE_STAR_IX
        // fall through
    default:
		qWarning() << "Unsupported shape type:" << type;
    }

#undef BM_SHAPE_TAG

    return shape;
}

bool BMShape::acceptsTrim() const {
    return false;
}

void BMShape::applyTrim(const BMTrimPath &trimmer) {
	if (trimmer.simultaneous()) {
		m_path = trimmer.trim(m_path);
	}
}

int BMShape::direction() const {
    return m_direction;
}

const QPainterPath &BMShape::path() const {
    return m_path;
}

} // namespace Lottie
