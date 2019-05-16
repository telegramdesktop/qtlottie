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
#include "bmbase.h"

#include "bmscene.h"

#include <QJsonObject>

namespace Lottie {

BMBase::BMBase(BMBase *parent) : m_parent(parent) {
}

BMBase::BMBase(BMBase *parent, const BMBase &other)
: m_type(other.m_type)
, m_hidden(other.m_hidden)
, m_name(other.m_name)
, m_matchName(other.m_matchName)
, m_autoOrient(other.m_autoOrient)
, m_parent(parent) {
	for (BMBase *child : other.m_children) {
		BMBase *clone = child->clone(this);
		appendChild(clone);
	}
}

BMBase::~BMBase() {
	qDeleteAll(m_children);
}

BMBase *BMBase::clone(BMBase *parent) const {
	return new BMBase(parent, *this);
}

QString BMBase::name() const {
	return m_name;
}

int BMBase::type() const {
	return m_type;
}

void BMBase::setType(int type) {
	m_type = type;
}

void BMBase::prependChild(BMBase *child) {
	m_children.push_back(child);
	if (const auto length = m_children.size(); length > 1) {
		qSwap(m_children[length - 1], m_children[length - 2]);
	}
}

void BMBase::appendChild(BMBase *child) {
	m_children.push_back(child);
}

void BMBase::updateProperties(int frame) {
	for (BMBase *child : children()) {
		if (child->active(frame)) {
			child->updateProperties(frame);
		}
	}
}

void BMBase::render(Renderer &renderer, int frame) const {
	for (BMBase *child : children()) {
		if (child->active(frame)) {
			child->render(renderer, frame);
		}
	}
}

void BMBase::resolveAssets(
		const std::function<BMAsset*(BMBase*, QString)> &resolver) {
	if (m_hidden) {
		return;
	}

	for (BMBase *child : children()) {
		if (child->m_hidden) {
			continue;
		}
		child->resolveAssets(resolver);
	}
}

BMScene *BMBase::resolveTopRoot() const {
	return m_parent->topRoot();
}

BMScene *BMBase::topRoot() const {
	if (!m_topRoot) {
		m_topRoot = resolveTopRoot();
	}
	return m_topRoot;
}

void BMBase::parse(const QJsonObject &definition) {
	m_hidden = definition.value(QStringLiteral("hd")).toBool(false);
	m_name = definition.value(QStringLiteral("nm")).toString();
	m_matchName = definition.value(QStringLiteral("mn")).toString();
	m_autoOrient = definition.value(QStringLiteral("ao")).toBool();

	if (m_autoOrient) {
		qWarning()
			<< "Element has auto-orientation set, but it is not supported";
	}
}

bool BMBase::active(int frame) const {
	return !m_hidden;
}

bool BMBase::hidden() const {
	return m_hidden;
}

} // namespace Lottie
