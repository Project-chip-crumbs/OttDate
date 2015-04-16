#ifndef LOG_H____
#define LOG_H____

#ifdef DEBUG
#define LOG_MESSAGE(fmt, ...) { fprintf(stderr,fmt,__VA_ARGS__); }

#define LOG_MSG(msg) { LOG_MESSAGE("%s: %s\n",__FUNCTION__,msg); }

#define LOG_MESSAGE_ENTER() \
{ \
  LOG_MESSAGE("%s: %s",__FUNCTION__," ENTER\n");\
}

#define LOG_MESSAGE_LEAVE() \
{ \
  LOG_MESSAGE("%s: %s",__FUNCTION__," LEAVE\n");\
}
#else
#define LOG_MESSAGE(fmt, ...)
#define LOG_MSG(msg)
#define LOG_MESSAGE_ENTER()
#define LOG_MESSAGE_LEAVE()
#endif

#endif // LOG_H____
