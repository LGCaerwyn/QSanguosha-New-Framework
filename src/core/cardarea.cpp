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

#include "cardarea.h"

CardArea::CardArea(CardArea::Type type, Player *owner)
    : m_type(type)
    , m_owner(owner)
{
}

bool CardArea::add(Card *card) {
    if (m_cards.contains(card))
        return false;
    m_cards << card;
    if (m_changeSignal)
        m_changeSignal();
    return true;
}

bool CardArea::add(const QList<Card *> &cards)
{
    int num = length();
    foreach (Card *card, cards) {
        if (m_cards.contains(card))
            continue;
        m_cards << card;
    }

    if (m_changeSignal && num != length())
            m_changeSignal();

    return num + cards.length() == length();
}

bool CardArea::remove(Card *card)
{
    if (m_cards.removeOne(card)) {
        if (m_changeSignal)
            m_changeSignal();
        return true;
    }

    return false;
}

bool CardArea::remove(const QList<Card *> &cards)
{
    int num = length();
    foreach (Card *card, cards)
        m_cards.removeOne(card);

    if (m_changeSignal && num != length())
            m_changeSignal();

    return num - cards.length() == length();
}

QList<const Card *> CardArea::cards() const
{
    QList<const Card *> result;
    foreach (const Card *card, m_cards)
        result << card;
    return result;
}