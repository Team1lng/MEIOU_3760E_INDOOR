#ifndef _TINYALSA_H
#define _TINYALSA_H

/**
 * tinycap(): Record func.
 *
 * @filename: record file name.
 * @card: card number.
 * @device: device number.
 * @channels: record channels.
 * @rate: sample rate.
 *
 *
 * */
int tinycap(const char *filename, unsigned int card, unsigned int device,
           unsigned int channels, unsigned int rate, unsigned int bits,
           unsigned int period_size, unsigned int period_count, unsigned int cap_time);

int tinyplay(const char *filename, unsigned int device, unsigned int card,
             unsigned int period_size, unsigned int period_count);
/**
 *
 * @start: tested channel data in frame, 
 *        eg: 16bit 4channels => 0 2 4 6
 *
 * */
void play_single_sample(const char *filename, unsigned int card, unsigned int device,
                        unsigned int channels, unsigned int rate, unsigned int bits,
                        unsigned int period_size, unsigned int period_count, int start);

#endif
