


//-------------------------------
// seção publica


typedef enum {

  LFAPP_INPUTFLAG_ENABLE = BIT(0),
  LFAPP_INPUTFLAG_ABBORT = BIT(1),
  LFAPP_INPUTFLAG_USER_1 = BIT(2),
  LFAPP_INPUTFLAG_USER_2 = BIT(3),
  LFAPP_INPUTFLAG_USER_3 = BIT(4),
  LFAPP_INPUTFLAG_USER_4 = BIT(5),
  LFAPP_INPUTFLAG_USER_5 = BIT(6),
  LFAPP_INPUTFLAG_USER_6 = BIT(7),
  LFAPP_INPUTFLAG_USER_7 = BIT(8),
  LFAPP_INPUTFLAG_USER_8 = BIT(9),
  LFAPP_INPUTFLAG_USER_9 = BIT(10),

//TODO: continuar ate o limite
 
  

} lfApp_flag_input_t;




typedef void* lfApp_h;



typedef struct lfApp_ {

  const unsigned int maxStates;
  unsigned int state;
  const char* name; //Apenas se o log estiver definido

  int (*pxLoop_fn[]) (lfApp_h app_context);  //< Isso deve ser um vetor, então cada posição representa um estado
  void (*pxAbort_fn) (lfApp_h app_context); //< Deve liberar toda a memoria alocada pela aplicação
  xEventGroup_t flags_xEventGroup;

  TaskHandle_t *pxCreatedTask;
  
}lfApp_t;


//-------------------------------

#ifdef FREE_RTOS
typedef EventBits_t flagMask_t; 
typedef int Timeoutms_t;
#define WHAIT_FOREVER (portMAX_DELAY)


static flagMask_t lfApp_whaitFlag(lfApp_h app_context, flagMask_t flagMask, bool whaitAll, bool abortOnTimeout, Timeout_t timeout_ms)
{
  assert(app_context != NULL);
  assert(app_context->abort != NULL);
  assert(app_context->flags_xEventGroup != NULL);
  
  TickType_t xTicksToWait = pMS_TO_TICKS(timeout_ms);

  if (whaitAll == false)
  {
    flagMask |= LFAPP_INPUTFLAG_ABBORT;
  }
  
  const EventBits_t flags_readed = xEventGroupWaitBits(
                      app_context->flags_xEventGroup,
                      flagMask,
                      false,
                      (BaseType_t) xWaitForAllBits,
                      xTicksToWait );
  
  if (flags_readed |= LFAPP_FLAG_ABBORT){
    app_context->abort("Abortando ao aguardar %s flags %d", whaitAll?"conjunto de","alguma das" ,flagMask);
    vTaskDelete(NULL);
  }

  if (flags_readed == 0 && abortOnTimeout)
  {
    app_context->abort("Timeout de %d ms ao aguardar %s flags %d",timeout_ms, whaitAll?"conjunto de":"alguma das" ,flagMask);
    vTaskDelete(NULL);
  }

  return(flags_readed);
  
}
#endif


void lf_app_disable(void* app_context)
{
  xEventGroupClearBits(app_context->flags_xEventGroup, LFAPP_INPUTFLAG_ENABLE);
}

void lf_app_abort(void* app_context)
{
  xEventGroupClearBits(app_context->flags_xEventGroup, LFAPP_FLAG_ABBORT);
}

void lf_app_loop (void* app_context)
{
  int nextState;

  while(true)
  {
    EventBits_t flags_readed = lfApp_whaitFlag(app_context, LFAPP_INPUTFLAG_ENABLE, false, false, 0);
    
    if ((flags_readed & LFAPP_INPUTFLAG_ENABLE) == 0x00)
    {
      vTaskSuspend(NULL);
    }
  
    app_context->state = nextState;
    nextState = app_context->loop[nextState](app_context); //Podem ser definidos multiplos estados aqui
    
    
    if (nextState > app_context->state)
    {
      ESP_LOGE(TAG, "Applicação %s em estado indefinido", app_context->name);
      lf_app_abort(app_context);
    }

  }
  
  ESP_LOGE(TAG, "Applicação %s saiu do loop", app_context->name);
  vTaskDelete(NULL);
}




