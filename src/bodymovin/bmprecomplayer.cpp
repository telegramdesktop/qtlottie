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

#include "bmprecomplayer_p.h"

#include <QJsonObject>
#include <QJsonArray>


#include "bmconstants_p.h"
#include "bmasset_p.h"
#include "bmbasictransform_p.h"
#include "lottierenderer_p.h"

QT_BEGIN_NAMESPACE

BMPreCompLayer::BMPreCompLayer(const BMPreCompLayer &other)
    : BMLayer(other)
{
    m_maskProperties = other.m_maskProperties;
    m_layerTransform = new BMBasicTransform(*other.m_layerTransform);
    m_layerTransform->setParent(this);
	if (other.m_layers) {
		m_layers = other.m_layers->clone();
		m_layers->setParent(nullptr);
	}
}

BMPreCompLayer::BMPreCompLayer(const QJsonObject &definition)
{
    m_type = BM_LAYER_PRECOMP_IX;

    BMLayer::parse(definition);
    if (m_hidden)
        return;

    qCDebug(lcLottieQtBodymovinParser) << "BMPreCompLayer::BMPreCompLayer()"
                                       << m_name;

    QJsonArray maskProps = definition.value(QLatin1String("maskProperties")).toArray();
    QJsonArray::const_iterator propIt = maskProps.constBegin();
    while (propIt != maskProps.constEnd()) {
        m_maskProperties.append((*propIt).toVariant().toInt());
        ++propIt;
    }

    QJsonObject trans = definition.value(QLatin1String("ks")).toObject();
    m_layerTransform = new BMBasicTransform(trans, this);

	m_refId = definition.value(QLatin1String("refId")).toString();

    if (m_maskProperties.length())
        qCWarning(lcLottieQtBodymovinParser)
            << "BM Shape Layer: mask properties found, but not supported"
            << m_maskProperties;
}

BMPreCompLayer::~BMPreCompLayer()
{
    if (m_layerTransform)
        delete m_layerTransform;
	if (m_layers)
		delete m_layers;
}

BMBase *BMPreCompLayer::clone() const
{
    return new BMPreCompLayer(*this);
}

void BMPreCompLayer::updateProperties(int frame)
{
    BMLayer::updateProperties(frame);

    m_layerTransform->updateProperties(frame);

	const auto layersFrame = frame - m_startFrame;
	if (m_layers && m_layers->active(layersFrame))
		m_layers->updateProperties(layersFrame);
}

void BMPreCompLayer::render(LottieRenderer &renderer, int frame) const
{
    renderer.saveState();

    renderEffects(renderer, frame);

    // In case there is a linked layer, apply its transform first
    // as it affects tranforms of this layer too
    if (BMLayer * ll = linkedLayer())
        ll->renderFullTransform(renderer, frame);

    renderer.render(*this);

    m_layerTransform->render(renderer, frame);

	const auto layersFrame = frame - m_startFrame;
	if (m_layers && m_layers->active(layersFrame))
		m_layers->render(renderer, layersFrame);

    renderer.restoreState();
}

void BMPreCompLayer::resolveAssets(const std::function<BMAsset*(QString)> &resolver) {
	if (m_layers)
		return;
	m_layers = resolver(m_refId);
	if (m_layers)
		m_layers->setParent(nullptr);
	else
		qCWarning(lcLottieQtBodymovinParser)
			<< "BM PreComp Layer: asset not found: "
			<< m_refId;
	BMLayer::resolveAssets(resolver);
}

QT_END_NAMESPACE
