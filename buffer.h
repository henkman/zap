// stdlib.h
// stdint.h
// string.h

typedef struct {
	uint8_t *ptr;
	size_t len, cap;
} Buffer;

static void buffer_init(Buffer *b, size_t cap)
{
	b->len = 0;
	b->cap = cap;
	b->ptr = malloc(sizeof(uint8_t) * b->cap);
}

static void buffer_append(Buffer *b, uint8_t c)
{
	if ((b->len + 1) >= b->cap) {
		b->cap *= 2;
		b->ptr = realloc(b->ptr, sizeof(uint8_t) * b->cap);
	}
	b->ptr[b->len] = c;
	b->len++;
}

static void buffer_destroy(Buffer *b) { free(b->ptr); }
static void buffer_write(Buffer *b, uint8_t *ptr, size_t len)
{
	size_t n = b->len + len;
	if (n >= b->cap) {
		size_t t = n;
		t--;
		t |= t >> 1;
		t |= t >> 2;
		t |= t >> 4;
		t |= t >> 8;
		t |= t >> 16;
		t++;
		b->cap = t;
		b->ptr = realloc(b->ptr, sizeof(uint8_t) * b->cap);
	}
	memcpy(&b->ptr[b->len], ptr, len);
	b->len += len;
}

static void buffer_writestring(Buffer *b, uint8_t *ptr)
{
	while (*ptr) {
		buffer_append(b, *ptr);
		ptr++;
	}
}