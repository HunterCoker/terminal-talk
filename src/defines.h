#define MIN_BUF_SIZE             1024
#define MAX_LINE_SIZE            128     /* BUF_SIZE >> 3 */

/* error types */
#define TT_ERROR_UNKOWN           0x1
#define TT_ERROR_THREAD           0x2
#define TT_ERROR_SOCKET           0x4

/* key codes */
#define TT_KEY_UNKOWN             -1
#define TT_KEY_ENTER              10
#define TT_KEY_SPACE              32
#define TT_KEY_APOSTROPHE         39  /* ' */
#define TT_KEY_COMMA              44  /* , */
#define TT_KEY_MINUS              45  /* - */
#define TT_KEY_PERIOD             46  /* . */
#define TT_KEY_SLASH              47  /* / */
#define TT_KEY_0                  48
#define TT_KEY_1                  49
#define TT_KEY_2                  50
#define TT_KEY_3                  51
#define TT_KEY_4                  52
#define TT_KEY_5                  53
#define TT_KEY_6                  54
#define TT_KEY_7                  55
#define TT_KEY_8                  56
#define TT_KEY_9                  57
#define TT_KEY_COLON              58  /* : */
#define TT_KEY_SEMICOLON          59  /* ; */
#define TT_KEY_EQUAL              61  /* = */
#define TT_KEY_LSQR_BRACKET       91  /* [ */
#define TT_KEY_BACKSLASH          92  /* \ */
#define TT_KEY_RSQR_BRACKET       93  /* ] */
#define TT_KEY_GRAVE_ACCENT       96  /* ` */
#define TT_KEY_ESCAPE             27
#define TT_KEY_TAB                9
#define TT_KEY_BACKSPACE          127
#define TT_KEY_DELETE             21
#define TT_KEY_RIGHT              261
#define TT_KEY_LEFT               260
#define TT_KEY_DOWN               258
#define TT_KEY_UP                 259
#define TT_KEY_HOME               1
#define TT_KEY_END                5
