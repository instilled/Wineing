package org.instilled.wineing.core;

import org.instilled.wineing.gen.WineingCtrlProto.Response;

public interface ResponseProcessor {

	void process(Response r);
}
