/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#include <linux/slab.h>
#else
#include <string.h>
#endif
#include <stdio.h>
#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */
   // define return enrty buffer pointer 
    struct aesd_buffer_entry * retrun_enrty = (struct aesd_buffer_entry *) malloc(sizeof(struct aesd_buffer_entry));
    // initialize as NULL pointer to avoid segmentation faults
    retrun_enrty = NULL;
    
    uint8_t index, count;
    // loop over the circular buffer 
    for(count=0, index = buffer->out_offs, retrun_enrty=&((buffer)->entry[index]); \
            count < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; \
            count++, index = (index + 1)%AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED, retrun_enrty=&((buffer)->entry[index])) {
        // check if this is the char_offset you looking for 
        // by check the size if this buffer
        if(char_offset < retrun_enrty->size) {
            // in this case the character we looking for inside this entry
            *entry_offset_byte_rtn = char_offset;
            return retrun_enrty; 
            break;               
        } else {
            // if not so check the next element but after decrimenting the char_offset
            char_offset -= retrun_enrty->size;
        }
    }
    return NULL;
}
/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */
   // first check if the circular buffer is full 
   if(!buffer->full) {
        // first assign the new entry buffer to circular buffer
        memcpy(&buffer->entry[buffer->in_offs], add_entry, sizeof(struct aesd_buffer_entry));

        //  increament in_offs then check if it exceed the array limits 
        if (++(buffer->in_offs)  == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) {
            // set the full variable true
            buffer->full = true;

            // reset in_offs variable 
            buffer->in_offs = buffer->out_offs;
        }

    } else {
        // in this case the circular buffer is full 

        // overwrite the oldest command 
        memcpy(&buffer->entry[buffer->out_offs], add_entry, sizeof(struct aesd_buffer_entry));
        // increament offset variables
        buffer->out_offs++;
        if(buffer->out_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) {
            buffer->out_offs = 0;
        }
        buffer->in_offs = buffer->out_offs;
   }
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
