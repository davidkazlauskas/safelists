#ifndef BASE64_Q11II03Q
#define BASE64_Q11II03Q

#include <cstddef>

int base64encode(const void* data_buf, size_t dataLength, char* result, size_t resultSize);
int base64decode(char *in, size_t inLen, unsigned char *out, size_t *outLen);

#endif /* end of include guard: BASE64_Q11II03Q */
