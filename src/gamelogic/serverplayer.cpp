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
#include "cardpattern.h"
#include "gamelogic.h"
#include "protocol.h"
#include "serverplayer.h"
#include "skill.h"

#include <croom.h>
#include <cserveragent.h>

ServerPlayer::ServerPlayer(GameLogic *logic, CServerAgent *agent)
    : Player(logic)
    , m_logic(logic)
    , m_room(logic->room())
    , m_agent(agent)
{
}

ServerPlayer::~ServerPlayer()
{
}

CServerAgent *ServerPlayer::agent() const
{
    return m_agent;
}

void ServerPlayer::setAgent(CServerAgent *agent)
{
    m_agent = agent;
}

CRoom *ServerPlayer::room() const
{
    if (m_room->isAbandoned())
        throw GameFinish;
    return m_room;
}

void ServerPlayer::drawCards(int n)
{
    CardsMoveStruct move;
    move.from.type = CardArea::DrawPile;
    move.from.direction = CardArea::Top;
    move.to.type = CardArea::Hand;
    move.to.owner = this;
    move.cards = m_logic->getDrawPileCards(n);

    m_logic->moveCards(move);
}

void ServerPlayer::recastCard(Card *card)
{
    CardsMoveStruct recast;
    recast.cards << card;
    recast.to.type = CardArea::Table;
    recast.isOpen = true;
    m_logic->moveCards(recast);

    const CardArea *table = m_logic->table();
    if (table->contains(card)) {
        CardsMoveStruct discard;
        discard.cards << card;
        discard.to.type = CardArea::DiscardPile;
        discard.isOpen = true;
        m_logic->moveCards(discard);
    }

    drawCards(1);
}

void ServerPlayer::showCard(Card *card)
{
    QVariantList cardData;
    cardData << card->id();

    QVariantMap data;
    data["from"] = id();
    data["cards"] = cardData;
    m_room->broadcastNotification(S_COMMAND_SHOW_CARD, data);
}

void ServerPlayer::showCards(const QList<Card *> &cards)
{
    QVariantList cardData;
    foreach (Card *card, cards)
        cardData << card->id();

    QVariantMap data;
    data["from"] = id();
    data["cards"] = cardData;
    m_agent->notify(S_COMMAND_SHOW_CARD, data);
}

void ServerPlayer::play()
{
    QList<Phase> phases;
    phases << RoundStart
           << Start
           << Judge
           << Draw
           << Play
           << Discard
           << Finish;
    play(phases);
}

void ServerPlayer::play(const QList<Player::Phase> &phases)
{
    PhaseChangeStruct change;
    foreach (Phase to, phases) {
        if (to == Inactive)
            break;
        change.from = phase();
        change.to = to;

        QVariant data = QVariant::fromValue(&change);
        bool skip = m_logic->trigger(PhaseChanging, this, data);

        setPhase(change.to);
        broadcastProperty("phase");

        if ((skip || isPhaseSkipped(change.to)) && !m_logic->trigger(PhaseSkipping, this, data))
            continue;

        if (!m_logic->trigger(PhaseStart, this))
            m_logic->trigger(PhaseProceeding, this);
        m_logic->trigger(PhaseEnd, this);
    }

    change.from = phase();
    change.to = Inactive;

    QVariant data = QVariant::fromValue(&change);
    m_logic->trigger(PhaseChanging, this, data);

    setPhase(change.to);
    broadcastProperty("phase");

    clearSkippedPhase();
}

void ServerPlayer::activate(CardUseStruct &use)
{
    int timeout = 15 * 1000;
    m_agent->request(S_COMMAND_USE_CARD, QVariant(), timeout);
    QVariant replyData = m_agent->waitForReply(timeout);
    if (replyData.isNull())
        return;
    QVariantMap reply = replyData.toMap();
    if (reply.isEmpty())
        return;

    use.from = this;

    uint cardId = reply["cardId"].toUInt();
    use.card = cardId > 0 ? m_logic->findCard(cardId) : nullptr;

    //@to-do: filter view as skills on server side

    QVariantList tos = reply["to"].toList();
    foreach (const QVariant &to, tos) {
        uint toId = to.toUInt();
        ServerPlayer *target = m_logic->findPlayer(toId);
        if (target)
            use.to << target;
    }
}

void ServerPlayer::showPrompt(const QString &message, int number)
{
    QVariantList data;
    data << message;
    data << number;
    m_agent->notify(S_COMMAND_SHOW_PROMPT, data);
}

void ServerPlayer::showPrompt(const QString &message, const QVariantList &args)
{
    QVariantList data;
    data << message;
    data << args;
    m_agent->notify(S_COMMAND_SHOW_PROMPT, data);
}

void ServerPlayer::showPrompt(const QString &message, const Card *card)
{
    QVariantList args;
    args << "card" << card->id();
    showPrompt(message, args);
}

void ServerPlayer::showPrompt(const QString &message, const ServerPlayer *from, const Card *card)
{
    QVariantList args;
    args << "player" << from->id();
    if (card)
        args << "card" << card->id();
    showPrompt(message, args);
}

void ServerPlayer::showPrompt(const QString &message, const ServerPlayer *p1, const ServerPlayer *p2, const Card *card)
{
    QVariantList args;
    args << "player" << p1->id();
    args << "player" << p2->id();
    if (card)
        args << "card" << card->id();
    showPrompt(message, args);
}

Event ServerPlayer::askForTriggerOrder(const QString &reason, const EventList &options, bool cancelable)
{
    //@todo:
    C_UNUSED(reason);
    C_UNUSED(options);
    C_UNUSED(cancelable);
    return Event();
}

Card *ServerPlayer::askForCard(const QString &pattern, bool optional)
{
    QVariantMap data;
    data["pattern"] = pattern;
    data["optional"] = optional;

    QVariant replyData;
    forever {
        m_agent->request(S_COMMAND_ASK_FOR_CARD, data, 15000);
        replyData = m_agent->waitForReply(15000);
        if (replyData.isNull())
            break;

        const QVariantMap reply = replyData.toMap();
        QList<Card *> cards = m_logic->findCards(reply["cards"]);
        //@to-do: filter cards with skill reply["skill"]
        if (cards.length() != 1)
            break;
        return cards.first();
    }

    if (!optional) {
        CardPattern p(pattern);

        QList<Card *> allCards = handcards()->cards();
        foreach (Card *card, allCards) {
            if (p.match(this, card))
                return card;
        }

        allCards = equips()->cards();
        foreach (Card *card, allCards) {
            if (p.match(this, card))
                return card;
        }
    }

    return nullptr;
}

QList<Card *> ServerPlayer::askForCards(const QString &pattern, int num, bool optional)
{
    return askForCards(pattern, num, num, optional);
}

QList<Card *> ServerPlayer::askForCards(const QString &pattern, int minNum, int maxNum, bool optional)
{
    if (maxNum < minNum)
        maxNum = minNum;

    QVariantMap data;
    data["pattern"] = pattern;
    data["minNum"] = minNum;
    data["maxNum"] = maxNum;
    data["optional"] = optional;

    m_agent->request(S_COMMAND_ASK_FOR_CARD, data, 15000);
    const QVariantMap replyData = m_agent->waitForReply(15000).toMap();

    if (optional) {
        if (replyData.isEmpty())
            return QList<Card *>();
        return m_logic->findCards(replyData["cards"]);
    } else {
        QList<Card *> cards = m_logic->findCards(replyData["cards"]);
        if (!optional) {
            if (cards.length() < minNum) {
                QList<Card *> allCards = handcards()->cards() + equips()->cards();
                CardPattern p(pattern);
                foreach (Card *card, allCards) {
                    if (!cards.contains(card) && p.match(this, card)) {
                        cards << card;
                        if (cards.length() >= minNum)
                            break;
                    }
                }
            } else if (cards.length() > maxNum) {
                cards = cards.mid(0, maxNum);
            }
        }
        return cards;
    }
}

Card *ServerPlayer::askToChooseCard(ServerPlayer *owner, const QString &areaFlag, bool handcardVisible)
{
    const CardArea *handcards = owner->handcards();
    const CardArea *equips = owner->equips();
    const CardArea *delayedTricks = owner->delayedTricks();

    QVariantMap data;

    if (areaFlag.contains('h')) {
        QVariantList handcardData;
        if (handcardVisible) {
            QList<Card *> cards = handcards->cards();
            foreach (const Card *card, cards)
                handcardData << card->id();
            data["handcards"] = handcardData;
        } else {
            data["handcards"] = owner->handcardNum();
        }
    }

    if (areaFlag.contains('e')) {
        QVariantList equipData;
        QList<Card *> cards = equips->cards();
        foreach (const Card *card, cards)
            equipData << card->id();
        data["equips"] = equipData;
    }

    if (areaFlag.contains('j')) {
        QVariantList trickData;
        QList<Card *> cards = delayedTricks->cards();
        foreach (const Card *card, cards)
            trickData << card->id();
        data["delayedTricks"] = trickData;
    }

    m_agent->request(S_COMMAND_CHOOSE_PLAYER_CARD, data, 15000);
    uint cardId = m_agent->waitForReply(15000).toUInt();
    if (cardId > 0) {
        if (areaFlag.contains('h') && handcardVisible) {
            Card *card = handcards->findCard(cardId);
            if (card)
                return card;
        }
        if (areaFlag.contains('e')) {
            Card *card = equips->findCard(cardId);
            if (card)
                return card;
        }
        if (areaFlag.contains('j')) {
            Card *card = delayedTricks->findCard(cardId);
            if (card)
                return card;
        }
    }

    if (areaFlag.contains('h') && handcards->length() > 0)
        return handcards->rand();

    if (areaFlag.contains('e') && equips->length() > 0)
        return equips->rand();

    if (areaFlag.contains('j') && delayedTricks->length() > 0)
        return delayedTricks->rand();

    return nullptr;
}

Card *ServerPlayer::askToUseCard(const QString &pattern, const QList<ServerPlayer *> &assignedTargets)
{
    QVariantMap data;
    data["pattern"] = pattern;

    QVariantList targetIds;
    foreach (ServerPlayer *target, assignedTargets)
        targetIds << target->id();
    data["assignedTargets"] = targetIds;

    m_agent->request(S_COMMAND_USE_CARD, data, 15000);
    const QVariantMap reply = m_agent->waitForReply(15000).toMap();
    if (reply.isEmpty())
        return nullptr;

    uint cardId = reply["cardId"].toUInt();
    Card *card = m_logic->findCard(cardId);
    if (card == nullptr)
        return nullptr;

    CardUseStruct use;
    use.from = this;
    use.card = card;

    QVariantList targets = reply["to"].toList();
    foreach (const QVariant &target, targets) {
        ServerPlayer *player = m_logic->findPlayer(target.toUInt());
        if (player)
            use.to << player;
    }
    foreach (ServerPlayer *target, assignedTargets) {
        if (!use.to.contains(target))
            return nullptr;
    }
    m_logic->useCard(use);
    return card;
}

bool ServerPlayer::askToInvokeSkill(const Skill *skill, const QVariant &data)
{
    Q_UNUSED(skill)
    Q_UNUSED(data)
    //@to-do: implement this
    return true;
}

void ServerPlayer::broadcastProperty(const char *name) const
{
    QVariantList data;
    data << id();
    data << name;
    data << property(name);
    m_room->broadcastNotification(S_COMMAND_UPDATE_PLAYER_PROPERTY, data);
}

void ServerPlayer::broadcastProperty(const char *name, const QVariant &value, ServerPlayer *except) const
{
    QVariantList data;
    data << id();
    data << name;
    data << value;
    m_room->broadcastNotification(S_COMMAND_UPDATE_PLAYER_PROPERTY, data, except->agent());
}

void ServerPlayer::unicastPropertyTo(const char *name, ServerPlayer *player)
{
    QVariantList data;
    data << id();
    data << name;
    data << property(name);
    CServerAgent *agent = player->agent();
    if (agent)
        agent->notify(S_COMMAND_UPDATE_PLAYER_PROPERTY, data);
}

void ServerPlayer::addCardHistory(const QString &name, int times)
{
    Player::addCardHistory(name, times);
    QVariantList data;
    data << name;
    data << times;

    m_agent->notify(S_COMMAND_ADD_CARD_HISTORY, data);
}

void ServerPlayer::clearCardHistory()
{
    Player::clearCardHistory();
    m_agent->notify(S_COMMAND_ADD_CARD_HISTORY);
}

void ServerPlayer::addHeadSkill(const Skill *skill)
{
    Player::addHeadSkill(skill);
    addTriggerSkill(skill);
}

void ServerPlayer::addDeputySkill(const Skill *skill)
{
    Player::addDeputySkill(skill);
    addTriggerSkill(skill);
}

void ServerPlayer::addAcquiredSkill(const Skill *skill)
{
    Player::addAcquiredSkill(skill);
    addTriggerSkill(skill);
}

void ServerPlayer::addTriggerSkill(const Skill *skill)
{
    if (skill->type() == Skill::TriggerType)
        m_logic->addEventHandler(static_cast<const TriggerSkill *>(skill));

    QList<const Skill *> subskills = skill->subskills();
    foreach (const Skill *subskill, subskills) {
        if (subskill->type() == Skill::TriggerType)
            m_logic->addEventHandler(static_cast<const TriggerSkill *>(subskill));
    }
}
