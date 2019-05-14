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

#include "bmbase_p.h"

#include <QLoggingCategory>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include "bmscene_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcLottieQtBodymovinParser, "qt.lottieqt.bodymovin.parser");
Q_LOGGING_CATEGORY(lcLottieQtBodymovinUpdate, "qt.lottieqt.bodymovin.update");
Q_LOGGING_CATEGORY(lcLottieQtBodymovinRender, "qt.lottieqt.bodymovin.render");

BMBase::BMBase(BMBase *parent) : m_parent(parent) {
}

BMBase::BMBase(BMBase *parent, const BMBase &other)
: m_definition(other.m_definition)
, m_type(other.m_type)
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

BMBase::~BMBase()
{
    qDeleteAll(m_children);
}

BMBase *BMBase::clone(BMBase *parent) const
{
    return new BMBase(parent, *this);
}

QString BMBase::name() const
{
    return m_name;
}

void BMBase::setName(const QString &name)
{
    m_name = name;
}

bool BMBase::setProperty(BMLiteral::PropertyType propertyName, QVariant value)
{
    for (BMBase *child : qAsConst(m_children)) {
        bool changed = child->setProperty(propertyName, value);
        if (changed)
            return true;
    }
    return false;
}

int BMBase::type() const
{
    return m_type;
}

void BMBase::setType(int type)
{
    m_type = type;
}

void BMBase::prependChild(BMBase *child)
{
    m_children.push_back(child);
    if (const auto length = m_children.size(); length > 1) {
        qSwap(m_children[length - 1], m_children[length - 2]);
    }
}

void BMBase::appendChild(BMBase *child)
{
    m_children.push_back(child);
}

BMBase *BMBase::findChild(const QString &childName)
{
    if (name() == childName)
        return this;

    BMBase *found = nullptr;
    for (BMBase *child : qAsConst(m_children)) {
        found = child->findChild(childName);
        if (found)
            break;
    }
    return found;
}

void BMBase::updateProperties(int frame)
{
    for (BMBase *child : qAsConst(m_children))
        if (child->active(frame))
            child->updateProperties(frame);
}

void BMBase::render(LottieRenderer &renderer, int frame) const
{
    for (BMBase *child : qAsConst(m_children))
        if (child->active(frame))
            child->render(renderer, frame);
}

void BMBase::resolveAssets(const std::function<BMAsset*(BMBase*, QString)> &resolver) {
    if (m_hidden)
        return;

    for (BMBase *child : qAsConst(m_children)) {
        if (child->m_hidden)
            continue;
        child->resolveAssets(resolver);
    }
}

BMScene *BMBase::resolveTopRoot() const
{
	return m_parent->topRoot();
}

BMScene *BMBase::topRoot() const
{
	if (!m_topRoot) {
		m_topRoot = resolveTopRoot();
	}
    return m_topRoot;
}

void BMBase::parse(const QJsonObject &definition)
{
    qCDebug(lcLottieQtBodymovinParser) << "BMBase::parse()";

    m_definition = definition;

    m_hidden = definition.value(QLatin1String("hd")).toBool(false);
    m_name = definition.value(QLatin1String("nm")).toString();
    m_matchName = definition.value(QLatin1String("mn")).toString();
    m_autoOrient = definition.value(QLatin1String("ao")).toBool();

    if (m_autoOrient)
        qCWarning(lcLottieQtBodymovinParser)
                << "Element has auto-orientation set, but it is not supported";
}

const QJsonObject &BMBase::definition() const
{
    return m_definition;
}

bool BMBase::active(int frame) const
{
    Q_UNUSED(frame);
    return !m_hidden;
}

bool BMBase::hidden() const
{
    return m_hidden;
}

const QJsonObject BMBase::resolveExpression(const QJsonObject &definition)
{
    QString expr = definition.value(QLatin1String("x")).toString();

    // If there is no expression, return the original object definition
    if (expr.isEmpty())
        return definition;

    QRegularExpression re(QStringLiteral("effect\\(\\'(.*?)\\'\\)\\(\\'(.*?)\\'\\)"));
    QRegularExpressionMatch match = re.match(expr);
    if (!match.hasMatch())
        return definition;

    QString effect = match.captured(1);
    QString elementName = match.captured(2);

    QJsonObject retVal = definition;

    if (BMBase *source = topRoot()->findChild(effect)) {
        if (source->children().length())
            retVal = source->children().at(0)->definition().value(QLatin1String("v")).toObject();
        else
            retVal = source->definition().value(QLatin1String("v")).toObject();
        if (source->children().length() > 1)
            qCWarning(lcLottieQtBodymovinParser) << "Effect source points"
                                                "to a group that has"
                                                "many children. The"
                                                "first is be picked";
    } else {
        qCWarning(lcLottieQtBodymovinParser) << "Failed to find specified effect" << effect;
    }

    // Let users of the json know that it is originated from expression,
    // so they can adjust their behavior accordingly
    retVal.insert(QLatin1String("fromExpression"), true);

    return retVal;
}

QT_END_NAMESPACE
