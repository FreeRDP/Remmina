
#ifdef __cplusplus
extern "C" {
#endif

int rp_kwallet_init(void);
void rp_kwallet_store_password(const char *key, const char *password);
char *rp_kwallet_get_password(const char *key);
void rp_kwallet_delete_password(const char *key);


#ifdef __cplusplus
}
#endif


