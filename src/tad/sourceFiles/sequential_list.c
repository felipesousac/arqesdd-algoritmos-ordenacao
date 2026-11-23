#include "../headerFiles/sequential_list.h"
#include <stdlib.h>
#include <string.h>

struct sequential_list {
  int size;
  int capacity;
  size_t element_size;
  void *elements;
};

SequentialList *seq_list_create_with_capacity(size_t element_size, int initial_capacity) {
  if (element_size == 0) {
    return NULL;
  }

  if (initial_capacity < 1) {
    initial_capacity = SEQ_LIST_INITIAL_CAPACITY;
  }

  SequentialList *list = (SequentialList *) malloc(sizeof(SequentialList));

  if (list == NULL) {
    return NULL;
  }

  list->size = 0;
  list->capacity = initial_capacity;
  list->element_size = element_size;
  list->elements = malloc((size_t)list->capacity * element_size);

  if (list->elements == NULL) {
    free(list);

    return NULL;
  }

  return list;
}

SequentialList *seq_list_create(size_t element_size) {
  return seq_list_create_with_capacity(element_size, SEQ_LIST_INITIAL_CAPACITY);
}

static int seq_list_expand(SequentialList *list) {
  int new_capacity = list->capacity * 2;

  if (new_capacity <= list->capacity || (size_t) new_capacity > (size_t) - 1 / list->element_size) {
    return 0;
  }

  void *new_elements = realloc(list->elements, (size_t) new_capacity * list->element_size);
  
  if (new_elements == NULL) {
    return 0;
  }

  list->elements = new_elements;
  list->capacity = new_capacity;

  return 1;
}

int seq_list_insert_end(SequentialList *list, const void *element) {
  if (list == NULL || element == NULL) {
    return 0;
  }

  if (list->size == list->capacity) {
    if (!seq_list_expand(list)) {
      return 0;
    }
  }

  memcpy((char *)list->elements + (size_t) list->size * list->element_size, element, list->element_size);
  
  list->size++;

  return 1;
}

int seq_list_insert_start(SequentialList *list, const void *element) {
  if (list == NULL || element == NULL) {
    return 0;
  }
  if (list->size == list->capacity) {
    if (!seq_list_expand(list)) {
      return 0;
    }
  }

  memmove((char *)list->elements + list->element_size, list->elements,
          (size_t)list->size * list->element_size);
  memcpy(list->elements, element, list->element_size);
  list->size++;
  return 1;
}

int seq_list_remove_at(SequentialList *list, int index) {
  if (list == NULL || index < 0 || index >= list->size) {
    return 0;
  }

  size_t remaining = (size_t)(list->size - index - 1);
  if (remaining > 0) {
    memmove((char *)list->elements + (size_t)index * list->element_size,
            (char *)list->elements + (size_t)(index + 1) * list->element_size,
            remaining * list->element_size);
  }
  list->size--;
  return 1;
}

int seq_list_remove_start(SequentialList *list) {
  if (list == NULL || list->size == 0) {
    return 0;
  }
  return seq_list_remove_at(list, 0);
}

int seq_list_remove_end(SequentialList *list) {
  if (list == NULL || list->size == 0) {
    return 0;
  }
  list->size--;
  return 1;
}

int seq_list_get(const SequentialList *list, int index, void *out_element) {
  if (list == NULL || out_element == NULL || index < 0 || index >= list->size) {
    return 0;
  }

  memcpy(out_element,
         (const char *)list->elements + (size_t)index * list->element_size,
         list->element_size);
  return 1;
}

int seq_list_set(SequentialList *list, int index, const void *element) {
  if (list == NULL || element == NULL || index < 0 || index >= list->size) {
    return 0;
  }

  memcpy((char *)list->elements + (size_t)index * list->element_size, element, list->element_size);

  return 1;
}

int seq_list_from_array(SequentialList *list, const void *elements, int count) {
  if (list == NULL || elements == NULL || count < 0) {
    return 0;
  }

  seq_list_clear(list);

  if (count == 0) {
    return 1;
  }

  if (count > list->capacity) {
    if ((size_t)count > (size_t)-1 / list->element_size) {
      return 0;
    }

    void *new_elements = realloc(list->elements, (size_t)count * list->element_size);
    
    if (new_elements == NULL) {
      return 0;
    }

    list->elements = new_elements;
    list->capacity = count;
  }

  memcpy(list->elements, elements, (size_t)count * list->element_size);
  list->size = count;

  return 1;
}

void *seq_list_at(const SequentialList *list, int index) {
  if (list == NULL || index < 0 || index >= list->size) {
    return NULL;
  }
  return (char *)list->elements + (size_t)index * list->element_size;
}

void *seq_list_data(const SequentialList *list) {
  if (list == NULL) {
    return NULL;
  }

  return list->elements;
}

int seq_list_size(const SequentialList *list) {
  if (list == NULL) {
    return -1;
  }

  return list->size;
}

int seq_list_capacity(const SequentialList *list) {
  if (list == NULL) {
    return -1;
  }
  return list->capacity;
}

size_t seq_list_element_size(const SequentialList *list) {
  if (list == NULL) {
    return 0;
  }
  return list->element_size;
}

int seq_list_is_full(const SequentialList *list) {
  if (list == NULL) {
    return -1;
  }
  return 0;
}

int seq_list_is_empty(const SequentialList *list) {
  if (list == NULL) {
    return -1;
  }
  return list->size == 0;
}

size_t seq_list_memory_usage(const SequentialList *list) {
  if (list == NULL) {
    return 0;
  }
  return sizeof(SequentialList) + (size_t)list->capacity * list->element_size;
}

void seq_list_clear(SequentialList *list) {
  if (list != NULL) {
    list->size = 0;
  }
}

void seq_list_destroy(SequentialList *list) {
  if (list != NULL) {
    free(list->elements);
    free(list);
  }
}
