
int main2(int argc, char **argv)
{
  // Wait for a client to send an initialization message.
  wchan_wait_for_client(ctx);
}

void* ctrl_send_thread(void *_ctx)
{
  WineingCtx *ctx = (WineingCtx *)_ctx;
  
  
  
  return NULL;
}

void* ctrl_recv_thread(void *_ctx)
{
  WineingCtx *ctx = (WineingCtx *)_ctx;
  
return NULL;
}

/**
 * The thread processing NxCore messages.
 */
void* market_thread(void *_ctx)
{
  WineingCtx *ctx = (WineingCtx *)_ctx;
return NULL;
}

