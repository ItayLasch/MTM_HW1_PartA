#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#define NO_SIZE -1
#include "map.h"

typedef struct map_node *Map_Node;

typedef enum List_Result
{
    SUCCESS,
    OUT_OF_MEMORY,
    NULL_ARGUMENT,
    FAILURE
} Node_Result;

Map_Node NodeCreate(Map map, MapKeyElement key_element, MapDataElement data_element);
Node_Result NodeRemove(Map_Node node, Map map);

struct map_node
{
    MapDataElement data_element;
    MapKeyElement key_element;
    struct map_node *next;
};

struct Map_t
{
    Map_Node head;
    copyMapDataElements copyDataElement;
    copyMapKeyElements copyKeyElement;
    freeMapDataElements freeDataElement;
    freeMapKeyElements freeKeyElement;
    compareMapKeyElements compareKeyElements;
    Map_Node iterator;
};

Map mapCreate(copyMapDataElements copyDataElement, copyMapKeyElements copyKeyElement, 
              freeMapDataElements freeDataElement,
              freeMapKeyElements freeKeyElement,
              compareMapKeyElements compareKeyElements)
{
    if (!copyDataElement || !copyKeyElement || !freeDataElement || !freeKeyElement || !compareKeyElements)
    {
        return NULL;
    }

    Map map = malloc(sizeof(*map));
    if (map == NULL)
    {
        return NULL;
    }
    map->head = NULL;
    map->copyDataElement = copyDataElement;
    map->copyKeyElement = copyKeyElement;
    map->freeDataElement = freeDataElement;
    map->freeKeyElement = freeKeyElement;
    map->compareKeyElements = compareKeyElements;
    map->iterator = NULL;
    return map;
}

void mapDestroy(Map map)
{
    if(map == NULL){
        return;
    }

    mapClear(map);
    free(map);
}

Map mapCopy(Map map)
{
    if (map == NULL)
    {
        return NULL;
    }

    Map map_copy = mapCreate(map->copyDataElement, map->copyKeyElement, map->freeDataElement,
                             map->freeKeyElement, map->compareKeyElements);
    if (map_copy == NULL)
    {
        return NULL;
    }
    map->iterator = map->head;
    while (map->iterator != NULL)
    {
        if (mapPut(map_copy, map->iterator->key_element, map->iterator->data_element) != MAP_SUCCESS)
        {
            mapDestroy(map_copy);
            return NULL;
        }
        map->iterator = map->iterator->next;
    }
    map->iterator = NULL;
    return map_copy;
}

int mapGetSize(Map map)
{
    int size = NO_SIZE;
    if (map == NULL)
    {
        return size;
    }
    map->iterator = map->head;
    while (map->iterator != NULL)
    {
        size++;
        map->iterator = map->iterator->next;
    }
    map->iterator = NULL;

    return (size + 1);
}

bool mapContains(Map map, MapKeyElement key_element)
{
    if (map == NULL || key_element == NULL)
    {
        return false;
    }
    map->iterator = map->head;
    if (map->iterator == NULL)
    {
        return false;
    }
    while (map->iterator != NULL && map->compareKeyElements(key_element, map->iterator->key_element) > 0)
    {
        map->iterator = map->iterator->next;
    }

    if (map->iterator != NULL && map->compareKeyElements(map->iterator->key_element, key_element) == 0)
    {
        map->iterator = NULL;
        return true;
    }
    map->iterator = NULL;
    return false;
}

MapResult mapPut(Map map, MapKeyElement keyElement, MapDataElement dataElement)
{
    if (map == NULL || keyElement == NULL || dataElement == NULL)
    {
        return MAP_NULL_ARGUMENT;
    }

    if (map->head == NULL)
    {
        map->head = NodeCreate(map, keyElement, dataElement);
        if (map->head == NULL)
        {
            return MAP_OUT_OF_MEMORY;
        }
        return MAP_SUCCESS;
    }

    if (mapContains(map, keyElement))
    {
        map->iterator = map->head;
        while (map->iterator != NULL && map->compareKeyElements(keyElement, map->iterator->key_element) > 0)
        {
            map->iterator = map->iterator->next;
        }
        if(map->iterator == NULL){
            return MAP_NULL_ARGUMENT;
        }
        MapDataElement tmpData = map->iterator->data_element;
        map->iterator->data_element = map->copyDataElement(dataElement);
        if (map->iterator->data_element == NULL)
        {
            map->iterator->data_element = tmpData;
            map->iterator = NULL;
            return MAP_OUT_OF_MEMORY;
        }
        map->freeDataElement(tmpData);
    }
    else
    {
        Map_Node new_node = NodeCreate(map, keyElement, dataElement);
        if (new_node == NULL)
        {
            map->iterator = NULL;
            return MAP_OUT_OF_MEMORY;
        }
        if (map->compareKeyElements(map->head->key_element, keyElement) > 0)
        {
            new_node->next = map->head;
            map->head = new_node;
        }
        else
        {
            map->iterator = map->head;
            while (map->iterator->next != NULL && 
                    map->compareKeyElements(keyElement, map->iterator->next->key_element) > 0) 
            {
                map->iterator = map->iterator->next;
            }
            new_node->next = map->iterator->next;
            map->iterator->next = new_node;
        }
    }
    map->iterator = NULL;
    return MAP_SUCCESS;
}

MapDataElement mapGet(Map map, MapKeyElement keyElement)
{
    if (map == NULL || keyElement == NULL)
    {
        return NULL;
    }
    Map_Node tmp = map->head;

    while (tmp != NULL && map->compareKeyElements(tmp->key_element, keyElement) != 0)
    {
        tmp = tmp->next;
    }
    if (tmp == NULL)
    {
        return NULL;
    }
    return tmp->data_element;
}

MapResult mapRemove(Map map, MapKeyElement keyElement)
{
    if (map == NULL || keyElement == NULL)
    {
        return MAP_NULL_ARGUMENT;
    }

    if (!mapContains(map, keyElement))
    {
        return MAP_ITEM_DOES_NOT_EXIST;
    }

    if (map->head != NULL && !map->compareKeyElements(map->head->key_element, keyElement))
    {
        Map_Node ptr = map->head;
        map->head = map->head->next;
        NodeRemove(ptr, map);
        return MAP_SUCCESS;
    }

    map->iterator = map->head;
    while(map->iterator->next != NULL && map->compareKeyElements(keyElement, map->iterator->next->key_element) != 0)
    {
        map->iterator = map->iterator->next;
    }
    Map_Node ptr = map->iterator->next;
    if (ptr != NULL && map->compareKeyElements(keyElement, ptr->key_element) == 0)
    {
        map->iterator->next = ptr->next;
        NodeRemove(ptr, map);
    }
    map->iterator = NULL;
    return MAP_SUCCESS;
}

MapResult mapClear(Map map)
{
    if (map == NULL)
    {
        return MAP_NULL_ARGUMENT;
    }

    Map_Node toDelete;
    while (map->head != NULL)
    {
        toDelete = map->head;
        map->head = map->head->next;
        NodeRemove(toDelete, map);
    }
    return MAP_SUCCESS;
}

MapKeyElement mapGetNext(Map map)
{
    if (map == NULL || map->iterator == NULL)
    {
        return NULL;
    }
    
    map->iterator = map->iterator->next;
    if (map->iterator == NULL)
    {
        return NULL;
    }
    return map->copyKeyElement(map->iterator->key_element);
}

MapKeyElement mapGetFirst(Map map)
{
    if (map == NULL || map->head == NULL)
    {
        return NULL;
    }

    map->iterator = map->head;
    return map->copyKeyElement(map->iterator->key_element);
}

//map node struct and fucntion**********************************************

Map_Node NodeCreate(Map map, MapKeyElement key_element, MapDataElement data_element)
{
    if (map == NULL || key_element == NULL || data_element == NULL)
    {
        return NULL;
    }

    Map_Node new_node = malloc(sizeof(*new_node));
    if (new_node == NULL)
    {
        return NULL;
    }

    new_node->data_element = map->copyDataElement(data_element);
    if (new_node->data_element == NULL)
    {
        free(new_node);
        return NULL;
    }

    new_node->key_element = map->copyKeyElement(key_element);
    if (new_node->key_element == NULL)
    {
        map->freeDataElement(new_node->data_element);
        free(new_node);
        return NULL;
    }

    new_node->next = NULL;
    return new_node;
}

Node_Result NodeRemove(Map_Node node, Map map)
{
    if (node == NULL || map == NULL)
    {
        return MAP_NULL_ARGUMENT;
    }
    if (node->key_element != NULL)
    {
        map->freeKeyElement(node->key_element);
    }
    if (node->data_element != NULL)
    {
        map->freeDataElement(node->data_element);
    }

    free(node);

    return SUCCESS;
}
