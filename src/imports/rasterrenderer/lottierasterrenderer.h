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
#include <QPainter>
#include <QStack>
#include <QRegion>

#include "lottierenderer.h"

class BMShape;
class QPainter;

class LottieRasterRenderer : public LottieRenderer {
public:
	explicit LottieRasterRenderer(QPainter *m_painter);

	void startMergeGeometry() override;
	void renderMergedGeometry() override;

	void saveState() override;
	void restoreState() override;

	void render(const BMLayer &layer) override;
	void render(const BMRect &rect) override;
	void render(const BMEllipse &ellipse) override;
	void render(const BMRound &round) override;
	void render(const BMFill &fill) override;
	void render(const BMGFill &shape) override;
	void render(const BMStroke &stroke) override;
	void render(const BMBasicTransform &transform) override;
	void render(const BMShapeTransform &transform) override;
	void render(const BMFreeFormShape &shape) override;
	void render(const BMTrimPath &trans) override;
	void render(const BMFillEffect &effect) override;
	void render(const BMRepeater &repeater) override;

	void render(const BMMaskShape &shape) override;
	void render(const BMMasks &masks) override;

protected:
	QPainter *m_painter = nullptr;
	QPainterPath m_unitedPath;
	// TODO: create a context to handle paths and effect
	// instead of pushing each to a stack independently
	QStack<QPainterPath> m_pathStack;
	QStack<const BMFillEffect*> m_fillEffectStack;
	const BMFillEffect *m_fillEffect = nullptr;
	const BMRepeaterTransform *m_repeaterTransform = nullptr;
	int m_repeatCount = 1;
	qreal m_repeatOffset = 0.0;
	bool m_buildingClipRegion = false;
	QPainterPath m_clipPath;
	int m_buildingMergedGeometry = 0;
	QPainterPath m_mergedGeometry;
	QStack<QPainterPath> m_mergedGeometryStack;
	bool m_buildingMaskRegion = false;
	QPainterPath m_maskPath;

private:
	void applyRepeaterTransform(int instance);
	void renderGeometry(const BMShape &geometry);

};
