package org.instilled.wineing.core;

public interface WineingRemoteAPI
{
    void start(ResponseProcessor p);

    void start(String tape, ResponseProcessor p);

    void stop(ResponseProcessor p);

    /**
     * Once the shutdown message was sent and the response processed it
     * is no longer possible to interact with {@link WineingRemoteAPI}.
     * 
     * @param p
     */
    void shutdown(ResponseProcessor p);

    void setDefaultResponseProcessor(ResponseProcessor p);
}