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

#include "bmnulllayer_p.h"

#include <QJsonObject>
#include <QJsonArray>


#include "bmconstants_p.h"
#include "bmbase_p.h"
#include "bmshape_p.h"
#include "bmtrimpath_p.h"
#include "bmbasictransform_p.h"
#include "lottierenderer_p.h"

QT_BEGIN_NAMESPACE

BMNullLayer::BMNullLayer(BMBase *parent) : BMLayer(parent) {
}

BMNullLayer::BMNullLayer(BMBase *parent, const BMNullLayer &other)
: BMLayer(parent, other) {
}

BMNullLayer::BMNullLayer(BMBase *parent, const QJsonObject &definition)
: BMLayer(parent) {
    m_type = BM_LAYER_NULL_IX;

    BMLayer::parse(definition);

    m_layerTransform.clearOpacity();

    qCDebug(lcLottieQtBodymovinParser) << "BMNullLayer::BMNullLayer()"
                                       << m_name;
}

BMNullLayer::~BMNullLayer() = default;

BMBase *BMNullLayer::clone(BMBase *parent) const
{
    return new BMNullLayer(parent, *this);
}

void BMNullLayer::render(LottieRenderer &renderer, int frame) const
{
}

QT_END_NAMESPACE
