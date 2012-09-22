package org.instilled.wineing.core;

public interface WineingRemoteAPI
{
    void start(ResponseProcessor p);

    void start(String tape, ResponseProcessor p);

    void stop(ResponseProcessor p);

    void shutdown(ResponseProcessor p);
    
    void setDefaultResponseProcessor(ResponseProcessor p);
}