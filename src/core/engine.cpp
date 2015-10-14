/********************************************************************
    Copyright (c) 2013-2015 - Mogara

    This file is part of QSanguosha.

    This game engine is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3.0
    of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    See the LICENSE file for more details.

    Mogara
*********************************************************************/

#include "card.h"
#include "engine.h"
#include "package.h"
#include "general.h"

Engine::Engine()
{
}

Engine *Engine::instance()
{
    static Engine engine;
    return &engine;
}

Engine::~Engine()
{
    foreach (Package *package, m_packages)
        delete package;
}

void Engine::addPackage(Package *package)
{
    m_packages << package;

    QList<const General *> generals = package->generals(true);
    foreach (const General *general, generals)
        m_generals.insert(general->name(), general);

    QList<const Card *> cards = package->cards();
    foreach (const Card *card, cards)
        m_cards.insert(card->id(), card);
}

const Package *Engine::package(const QString &name) const
{
    foreach (Package *package, m_packages) {
        if (package->name() == name)
            return package;
    }

    return nullptr;
}

QList<const Package *> Engine::packages() const
{
    QList<const Package *> packages;
    packages.reserve(m_packages.length());
    foreach (Package *package, m_packages)
        packages << package;
    return packages;
}

QList<const General *> Engine::getGenerals(bool includeHidden) const
{
    QList<const General *> generals;
    foreach (const Package *package, m_packages)
        generals << package->generals(includeHidden);
    return generals;
}

QList<const Card *> Engine::getCards() const
{
    QList<const Card *> cards;
    foreach (const Package *package, m_packages)
        cards << package->cards();
    return cards;
}
