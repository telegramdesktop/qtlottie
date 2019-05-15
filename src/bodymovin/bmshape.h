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
#pragma once

#include <QPainterPath>

#include "bmbase.h"
#include "bmproperty.h"

namespace Lottie {

class BMFill;
class BMStroke;
class BMTrimPath;

#define BM_SHAPE_ANY_TYPE_IX -1
#define BM_SHAPE_ELLIPSE_IX  0x00000
#define BM_SHAPE_FILL_IX     0x00001
#define BM_SHAPE_GFILL_IX    0x00002
#define BM_SHAPE_GSTROKE_IX  0x00003
#define BM_SHAPE_GROUP_IX    0x00004
#define BM_SHAPE_RECT_IX     0x00005
#define BM_SHAPE_ROUND_IX    0x00006
#define BM_SHAPE_SHAPE_IX    0x00007
#define BM_SHAPE_STAR_IX     0x00008
#define BM_SHAPE_STROKE_IX   0x00009
#define BM_SHAPE_TRIM_IX     0x0000A
#define BM_SHAPE_TRANS_IX    0x0000B
#define BM_SHAPE_REPEATER_IX 0x0000C

class BMShape : public BMBase {
public:
	BMShape(BMBase *parent);
	BMShape(BMBase *parent, const BMShape &other);

	BMBase *clone(BMBase *parent) const override;

	static BMShape *construct(BMBase *parent, QJsonObject definition);

	virtual const QPainterPath &path() const;
	virtual bool acceptsTrim() const;
	virtual void applyTrim(const BMTrimPath& trimmer);

	int direction() const;

protected:
	QPainterPath m_path;
	BMTrimPath *m_appliedTrim = nullptr;
	int m_direction = 0;

};

} // namespace Lottie
