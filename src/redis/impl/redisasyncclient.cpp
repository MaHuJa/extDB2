/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 */

#pragma once


RedisAsyncClient::RedisAsyncClient(boost::asio::io_service &ioService)
    : pimpl(std::make_shared<RedisClientImpl>(boost::ref(ioService)))
{
    pimpl->errorHandler = boost::bind(&RedisClientImpl::defaulErrorHandler,
                                      pimpl, _1);
}

RedisAsyncClient::~RedisAsyncClient()
{
    pimpl->close();
}

void RedisAsyncClient::connect(const boost::asio::ip::address &address,
                               unsigned short port,
                               const boost::function<void(bool, const std::string &)> &handler)
{
    boost::asio::ip::tcp::endpoint endpoint(address, port);
    connect(endpoint, handler);
}

void RedisAsyncClient::connect(const boost::asio::ip::tcp::endpoint &endpoint,
                               const boost::function<void(bool, const std::string &)> &handler)
{
    pimpl->socket.async_connect(endpoint, boost::bind(&RedisClientImpl::handleAsyncConnect,
                                                      pimpl, _1, handler));
}


void RedisAsyncClient::installErrorHandler(
        const boost::function<void(const std::string &)> &handler)
{
    pimpl->errorHandler = handler;
}

void RedisAsyncClient::command(std::vector<std::string> &items, const boost::function<void(const RedisValue &)> &handler)
{
    if(stateValid())
    {
        pimpl->post(boost::bind(&RedisClientImpl::doAsyncCommand, pimpl, items, handler));
    }
}

RedisAsyncClient::Handle RedisAsyncClient::subscribe(
        const std::string &channel,
        const boost::function<void(const std::string &msg)> &msgHandler,
        const boost::function<void(const RedisValue &)> &handler)
{
    assert( pimpl->state == RedisClientImpl::Connected ||
            pimpl->state == RedisClientImpl::Subscribed);

    static std::string subscribe = "SUBSCRIBE";

    if( pimpl->state == RedisClientImpl::Connected || pimpl->state == RedisClientImpl::Subscribed )
    {
        Handle handle = {pimpl->subscribeSeq++, channel};

        std::vector<std::string> items(2);
        items[0] = subscribe;
        items[1] = channel;

        pimpl->post(boost::bind(&RedisClientImpl::doAsyncCommand, pimpl, items, handler));
        pimpl->msgHandlers.insert(std::make_pair(channel, std::make_pair(handle.id, msgHandler)));
        pimpl->state = RedisClientImpl::Subscribed;

        return handle;
    }
    else
    {
        std::stringstream ss;

        ss << "RedisAsyncClient::command called with invalid state "
           << pimpl->state;

        pimpl->errorHandler(ss.str());
        return Handle();
    }
}

void RedisAsyncClient::unsubscribe(const Handle &handle)
{
#ifdef DEBUG
    static int recursion = 0;
    assert( recursion++ == 0 );
#endif

    assert( pimpl->state == RedisClientImpl::Connected ||
            pimpl->state == RedisClientImpl::Subscribed);

    static std::string unsubscribe = "UNSUBSCRIBE";

    if( pimpl->state == RedisClientImpl::Connected ||
            pimpl->state == RedisClientImpl::Subscribed )
    {
        // Remove subscribe-handler
        typedef RedisClientImpl::MsgHandlersMap::iterator iterator;
        std::pair<iterator, iterator> pair = pimpl->msgHandlers.equal_range(handle.channel);

        for(iterator it = pair.first; it != pair.second;)
        {
            if( it->second.first == handle.id )
            {
                pimpl->msgHandlers.erase(it++);
            }
            else
            {
                ++it;
            }
        }

        std::vector<std::string> items(2);
        items[0] = unsubscribe;
        items[1] = handle.channel;

        // Unsubscribe command for Redis
        pimpl->post(boost::bind(&RedisClientImpl::doAsyncCommand, pimpl, items, dummyHandler));
    }
    else
    {
        std::stringstream ss;

        ss << "RedisAsyncClient::command called with invalid state "
           << pimpl->state;

#ifdef DEBUG
        --recursion;
#endif
        pimpl->errorHandler(ss.str());
        return;
    }

#ifdef DEBUG
    --recursion;
#endif
}

void RedisAsyncClient::singleShotSubscribe(const std::string &channel,
                                      const boost::function<void(const std::string &msg)> &msgHandler,
                                      const boost::function<void(const RedisValue &)> &handler)
{
    assert( pimpl->state == RedisClientImpl::Connected ||
            pimpl->state == RedisClientImpl::Subscribed);

    static std::string subscribe = "SUBSCRIBE";

    if( pimpl->state == RedisClientImpl::Connected ||
            pimpl->state == RedisClientImpl::Subscribed )
    {
        std::vector<std::string> items(2);
        items[0] = subscribe;
        items[1] = channel;

        pimpl->post(boost::bind(&RedisClientImpl::doAsyncCommand, pimpl, items, handler));
        pimpl->singleShotMsgHandlers.insert(std::make_pair(channel, msgHandler));
        pimpl->state = RedisClientImpl::Subscribed;
    }
    else
    {
        std::stringstream ss;

        ss << "RedisAsyncClient::command called with invalid state "
           << pimpl->state;

        pimpl->errorHandler(ss.str());
    }
}


void RedisAsyncClient::publish(const std::string &channel, const std::string &msg,
                          const boost::function<void(const RedisValue &)> &handler)
{
    assert( pimpl->state == RedisClientImpl::Connected );

    static std::string publish = "PUBLISH";

    if( pimpl->state == RedisClientImpl::Connected )
    {
        std::vector<std::string> items(3);

        items[0] = publish;
        items[1] = channel;
        items[2] = msg;

        pimpl->post(boost::bind(&RedisClientImpl::doAsyncCommand, pimpl, items, handler));
    }
    else
    {
        std::stringstream ss;

        ss << "RedisAsyncClient::command called with invalid state "
           << pimpl->state;

        pimpl->errorHandler(ss.str());
    }
}

bool RedisAsyncClient::stateValid() const
{
    assert( pimpl->state == RedisClientImpl::Connected );

    if( pimpl->state != RedisClientImpl::Connected )
    {
        std::stringstream ss;

        ss << "RedisAsyncClient::command called with invalid state "
           << pimpl->state;

        pimpl->errorHandler(ss.str());
        return false;
    }

    return true;
}
