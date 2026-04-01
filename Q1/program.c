#include <stdio.h>
#include <stdlib.h>

int g_line_checksum;

void fill_sequence(int *buf, int n)
{
	int i;

	for (i = 0; i < n; i++) {
		if ((i & 1) == 0)
			buf[i] = i * 3;
		else
			buf[i] = -i;
	}
}

int sum_positive(const int *arr, int n)
{
	int i;
	int acc = 0;

	for (i = 0; i < n; i++) {
		if (arr[i] > 0)
			acc += arr[i];
	}
	return acc;
}

void format_result(int value)
{
	char *heap_buf;
	size_t len;

	heap_buf = malloc(128);
	if (heap_buf == NULL) {
		puts("malloc failed");
		return;
	}
	/* Write formatted text into heap-allocated buffer */
	len = (size_t)snprintf(heap_buf, 128, "computed_sum=%d\n", value);
	if (len < 128)
		heap_buf[len] = '\0';
	g_line_checksum += (int)len;
	printf("%s", heap_buf);
	free(heap_buf);
}

int main(void)
{
	enum { N = 10 };
	int stack_arr[N];
	int total;

	g_line_checksum = 0x2a;
	fill_sequence(stack_arr, N);
	total = sum_positive(stack_arr, N);
	format_result(total);
	return 0;
}
