/* dumps information about tracks and its containing events to the terminal's
 * standard output. not all meta events will be fully decoded and sysex events
 * will only show their length, but is otherwise "functional enough"
 * 
 * this requires a midi file named test.mid to be in the same working directory
 * that the program is ran from (this is only really meant to be a test program
 * so i haven't bothered reading from argv for a file path) */

#include <mf_construct.h>
#include <mf_parse.h>

#include <stdio.h>

const static char *COMMON_STATUS_NAMES[] = {
    "note off",
    "note on",
    "poly key pressure",
    "control change",
    "program change",
    "channel pressure",
    "pitch bend"
};

const static char *META_EVENT_NAMES[128];
const static char *CC_NAMES[128];

static void fill_cc_name_table(void) {
    for (int i = 0; i < 128; i++)
        CC_NAMES[i] = "?";
    
    CC_NAMES[0] = "bank select";
    CC_NAMES[1] = "modulation";
    CC_NAMES[2] = "breath controller";
    CC_NAMES[4] = "foot controller";
    CC_NAMES[5] = "portamento time";
    CC_NAMES[6] = "data entry msb";
    CC_NAMES[7] = "channel volume";
    CC_NAMES[8] = "balance";
    CC_NAMES[10] = "panning";
    CC_NAMES[11] = "expression";
    CC_NAMES[12] = "effect 1";
    CC_NAMES[13] = "effect 2";
    CC_NAMES[16] = "general purpose 1";
    CC_NAMES[17] = "general purpose 2";
    CC_NAMES[18] = "general purpose 3";
    CC_NAMES[19] = "general purpose 4";

    CC_NAMES[32 + 0] = "bank select lsb";
    CC_NAMES[32 + 1] = "modulation lsb";
    CC_NAMES[32 + 2] = "breath controller lsb";
    CC_NAMES[32 + 4] = "foot controller lsb";
    CC_NAMES[32 + 5] = "portamento time lsb";
    CC_NAMES[32 + 6] = "data entry lsb";
    CC_NAMES[32 + 7] = "channel volume lsb";
    CC_NAMES[32 + 8] = "balance lsb";
    CC_NAMES[32 + 10] = "panning lsb";
    CC_NAMES[32 + 11] = "expression lsb";
    CC_NAMES[32 + 12] = "effect 1 lsb";
    CC_NAMES[32 + 13] = "effect 2 lsb";
    CC_NAMES[32 + 16] = "general purpose 1 lsb";
    CC_NAMES[32 + 17] = "general purpose 2 lsb";
    CC_NAMES[32 + 18] = "general purpose 3 lsb";
    CC_NAMES[32 + 19] = "general purpose 4 lsb";

    CC_NAMES[64] = "sustain pedal";
    CC_NAMES[65] = "portamento toggle";
    CC_NAMES[66] = "sostenuto";
    CC_NAMES[67] = "soft pedal";
    CC_NAMES[68] = "legato footswitch";
    CC_NAMES[69] = "hold 2";
    CC_NAMES[70] = "sound ctrlr. 1 / sound var.";
    CC_NAMES[71] = "sound ctrlr. 2 / timbre";   
    CC_NAMES[72] = "sound ctrlr. 3 / rel. time";
    CC_NAMES[73] = "sound ctrlr. 4 / atk. time";
    CC_NAMES[74] = "sound ctrlr. 5 / brightness";
    CC_NAMES[75] = "sound ctrlr. 6";
    CC_NAMES[76] = "sound ctrlr. 7";
    CC_NAMES[77] = "sound ctrlr. 8";
    CC_NAMES[78] = "sound ctrlr. 9";
    CC_NAMES[79] = "sound ctrlr. 10";
    CC_NAMES[80] = "general purpose 5";
    CC_NAMES[81] = "general purpose 6";
    CC_NAMES[82] = "general purpose 7";
    CC_NAMES[83] = "general purpose 8";
    CC_NAMES[84] = "portamento control";
    CC_NAMES[91] = "effect 1 / efx depth";
    CC_NAMES[92] = "effect 2 / trem. depth";
    CC_NAMES[93] = "effect 3 / chor. depth";
    CC_NAMES[94] = "effect 4 / cele. depth";
    CC_NAMES[95] = "effect 5 / phsr. depth";    
    CC_NAMES[96] = "data increment";
    CC_NAMES[97] = "data decrement";

    CC_NAMES[98] = "nrpn lsb";
    CC_NAMES[99] = "nrpn msb";
    CC_NAMES[100] = "rpn lsb";
    CC_NAMES[101] = "rpn msb";
}

static void fill_meta_event_name_table(void) {
    for (int i = 0; i < 128; i++)
        META_EVENT_NAMES[i] = "?";

    META_EVENT_NAMES[0x00] = "sequence number";
    META_EVENT_NAMES[0x01] = "text";
    META_EVENT_NAMES[0x02] = "copyright notice";
    META_EVENT_NAMES[0x03] = "sequence / track name";
    META_EVENT_NAMES[0x04] = "instrument name";
    META_EVENT_NAMES[0x05] = "lyric";
    META_EVENT_NAMES[0x06] = "marker";
    META_EVENT_NAMES[0x07] = "cue point";
    META_EVENT_NAMES[0x20] = "midi channel prefix";
    META_EVENT_NAMES[0x2f] = "end of track";
    META_EVENT_NAMES[0x51] = "tempo set";
    META_EVENT_NAMES[0x54] = "smpte offset";
    META_EVENT_NAMES[0x58] = "time signature";
    META_EVENT_NAMES[0x59] = "key signature";
    META_EVENT_NAMES[0x7f] = "sequencer specific";
}

static void print_status_name(uint8_t status) {
    if (status >= 0x80 && status <= 0xef)
    {
        printf("%s (ch%d)",
               COMMON_STATUS_NAMES[(status & 0x70) >> 4], 
               status & 0x0f);
    }
    else if (status == 0xf0)
        printf("sysex");
    else if (status == 0xf7)
        printf("sysex (continuation packet ?)");
    else if (status == 0xff)
        printf("meta");
    else 
        printf("unknown?");
}

static void print_note_event_data(mf_midi_event_data_basic data) {
    if (data.parm2 > 0) printf("note %d, vel %d", data.parm1, data.parm2);
    else printf("note %d (OFF)", data.parm1);
}

static void print_key_pressure_data(mf_midi_event_data_basic data) {
    printf("note %d, pressure %d", data.parm1, data.parm2);
}

static void print_pitch_bend_data(mf_midi_event_data_basic data) {
    printf("%d", (uint16_t)((data.parm1 << 8) | data.parm1));
}

static void print_1parm_data(mf_midi_event_data_basic data) {
    printf("%d", data.parm1);
}

static void print_cc_data(mf_midi_event_data_basic data) {
    printf("%s (%d) -> %d", CC_NAMES[data.parm1], data.parm1, 
           data.parm2); 
}

static void print_meta_data(mf_midi_event_data_meta data) {
    printf("[%s/%02x] ", META_EVENT_NAMES[data.type], data.type);

    /* text events */
    if (data.type >= 0x01 && data.type <= 0x07)
        printf("\"%s\"", data.data);
    else
        printf("(%d bytes)", data.length);
}

int main(void) {
    fill_meta_event_name_table();
    fill_cc_name_table();

    mf_midi_file *file = mf_create_from_file("test.mid");
    if (file == NULL) {
        fprintf(stderr, "failed to open file\n");
        return 1;
    }

    if (!mf_check_and_read_header(file)) {
        fprintf(stderr, "failed to read header\n");
        mf_free(file);
        return 1;
    }

    if (!mf_read_all_tracks(file)) {
        fprintf(stderr, "failed to read tracks\n");
        mf_free(file);
        return 1;
    }

    printf("%d tracks\n", file->nr_tracks);
    printf("time division: %d ppq\n", file->time_division);

    for (int i = 0; i < file->nr_tracks; i++) {
        mf_midi_track track = file->tracks[i];
        printf("track %d:\n", i);
        printf("\t%d bytes\n", track.byte_length);
        printf("\t%zu events\n", track.nr_events);

        for (mf_midi_event_linked_list *e = track.events; 
             e != NULL; e = e->next_event) {
            mf_midi_event event = e->cur_event;
         
            printf("\t");
            if (event.delta > 0) printf("(+%d tick(s)) ", event.delta);
            print_status_name(event.status);
            printf(": ");
         
            if (event.status >= 0x80 && event.status <= 0x9f)
                print_note_event_data(event.data_basic);
            else if (event.status >= 0xa0 && event.status <= 0xaf)
                print_key_pressure_data(event.data_basic);
            else if (event.status >= 0xb0 && event.status <= 0xbf)
                print_cc_data(event.data_basic);
            /* program change / aftertouch */
            else if (event.status >= 0xc0 && event.status <= 0xdf)
                print_1parm_data(event.data_basic);
            else if (event.status >= 0xe0 && event.status <= 0xef)
                print_pitch_bend_data(event.data_basic);
            else if (event.status == 0xf0 || event.status == 0xf9)
                printf("%d bytes", event.data_sysex.length);
            else if (event.status == 0xff)
                print_meta_data(event.data_meta);

            printf("\n");
        }
    }

    mf_free(file);

    return 0;
}
