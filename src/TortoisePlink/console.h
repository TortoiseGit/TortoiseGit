/*
 * Common pieces between the platform console frontend modules.
 */

char *hk_absentmsg_common(const char *host, int port,
                          const char *keytype, const char *fingerprint);
extern const char hk_absentmsg_interactive_intro[];
extern const char hk_absentmsg_interactive_prompt[];

char *hk_wrongmsg_common(const char *host, int port,
                         const char *keytype, const char *fingerprint);
extern const char hk_wrongmsg_interactive_intro[];
extern const char hk_wrongmsg_interactive_prompt[];

extern const char weakcrypto_msg_common_fmt[];

extern const char weakhk_msg_common_fmt[];

extern const char console_continue_prompt[];
extern const char console_abandoned_msg[];
