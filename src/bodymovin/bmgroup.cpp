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
#include "bmgroup.h"

#include "bmbase.h"
#include "bmshape.h"
#include "bmtrimpath.h"
#include "bmbasictransform.h"
#include "renderer.h"

#include <QJsonObject>
#include <QJsonArray>

namespace Lottie {

BMGroup::BMGroup(BMBase *parent) : BMShape(parent) {
}

BMGroup::BMGroup(BMBase *parent, const BMGroup &other)
: BMShape(parent, other) {
}

BMGroup::BMGroup(BMBase *parent, const QJsonObject &definition)
: BMShape(parent) {
	parse(definition);
}

BMBase *BMGroup::clone(BMBase *parent) const {
	return new BMGroup(parent, *this);
}

void BMGroup::parse(const QJsonObject &definition) {
	BMBase::parse(definition);
	if (m_hidden) {
		return;
	}

	QJsonArray groupItems = definition.value(QLatin1String("it")).toArray();
	QJsonArray::const_iterator itemIt = groupItems.constEnd();
	while (itemIt != groupItems.constBegin()) {
		itemIt--;
		BMShape *shape = BMShape::construct(this, (*itemIt).toObject());
		if (shape) {
			// Transform affects how group contents are drawn.
			// It must be traversed first when drawing
			if (shape->type() == BM_SHAPE_TRANS_IX) {
				prependChild(shape);
			} else {
				appendChild(shape);
			}
		}
	}
}

void BMGroup::updateProperties(int frame) {
	BMShape::updateProperties(frame);

	for (BMBase *child : children()) {
		if (!child->active(frame)) {
			continue;
		}

		BMShape *shape = static_cast<BMShape*>(child);
		if (shape->type() == BM_SHAPE_TRIM_IX) {
			BMTrimPath *trim = static_cast<BMTrimPath*>(shape);
			if (m_appliedTrim) {
				m_appliedTrim->applyTrim(*trim);
			} else {
				m_appliedTrim = trim;
			}
		} else if (m_appliedTrim && shape->acceptsTrim()) {
			shape->applyTrim(*m_appliedTrim);
		}
	}
}

void BMGroup::render(Renderer &renderer, int frame) const {
	renderer.saveState();

	if (m_appliedTrim && !m_appliedTrim->hidden()) {
		if (m_appliedTrim->simultaneous()) {
			renderer.setTrimmingState(Renderer::Simultaneous);
		} else {
			renderer.setTrimmingState(Renderer::Individual);
		}
	} else {
		renderer.setTrimmingState(Renderer::Off);
	}

	renderer.startMergeGeometry();
	for (BMBase *child : children()) {
		if (child->active(frame)) {
			child->render(renderer, frame);
		}
	}
	renderer.renderMergedGeometry();

	if (m_appliedTrim
		&& !m_appliedTrim->hidden()
		&& !m_appliedTrim->simultaneous()) {
		m_appliedTrim->render(renderer, frame);
	}

	renderer.restoreState();
}

bool BMGroup::acceptsTrim() const {
	return true;
}

void BMGroup::applyTrim(const BMTrimPath &trimmer) {
	Q_ASSERT_X(!m_appliedTrim, "BMGroup", "A trim already assigned");

	m_appliedTrim = static_cast<BMTrimPath*>(trimmer.clone(this));
	// Setting a friendly name helps in testing
	//m_appliedTrim->setName(QStringLiteral("Inherited from") + trimmer.name());

	for (BMBase *child : children()) {
		BMShape *shape = static_cast<BMShape*>(child);
		if (shape->acceptsTrim()) {
			shape->applyTrim(*m_appliedTrim);
		}
	}
}

} // namespace Lottie
