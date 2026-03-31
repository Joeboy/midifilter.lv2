MFD_FILTER(chromaticcluster)

#ifdef MX_TTF

	mflt:chromaticcluster
	TTF_DEFAULTDEF("MIDI Chromatic Cluster", "MIDI Chromatic Cluster")
	, TTF_IPORT(0, "channelf", "Filter Channel",  0, 16, 0,
			PORTENUMZ("Any")
			DOC_CHANF)
	, TTF_IPORT(1, "note1", "1st Note",  -24, 24,  0,
			lv2:portProperty lv2:integer;
			lv2:scalePoint [ rdfs:label "off" ; rdf:value 0 ] ;
			)
	, TTF_IPORT(2, "note2", "2nd Note",  -24, 24,  0,
			lv2:portProperty lv2:integer;
			lv2:scalePoint [ rdfs:label "off" ; rdf:value 0 ] ;
			)
	, TTF_IPORT(3, "note3", "3rd Note",  -24, 24,  0,
			lv2:portProperty lv2:integer;
			lv2:scalePoint [ rdfs:label "off" ; rdf:value 0 ] ;
			)
	, TTF_IPORT(4, "note4", "4th Note",  -24, 24,  0,
			lv2:portProperty lv2:integer;
			lv2:scalePoint [ rdfs:label "off" ; rdf:value 0 ] ;
			)
	, TTF_IPORT(5, "note5", "5th Note",  -24, 24,  0,
			lv2:portProperty lv2:integer;
			lv2:scalePoint [ rdfs:label "off" ; rdf:value 0 ] ;
			)
	, TTF_IPORT(6, "note6", "6th Note",  -24, 24,  0,
			lv2:portProperty lv2:integer;
			lv2:scalePoint [ rdfs:label "off" ; rdf:value 0 ] ;
			)
	, TTF_IPORT(7, "note7", "7th Note",  -24, 24,  0,
			lv2:portProperty lv2:integer;
			lv2:scalePoint [ rdfs:label "off" ; rdf:value 0 ] ;
			)
	, TTF_IPORT(8, "note8", "8th Note",  -24, 24,  0,
			lv2:portProperty lv2:integer;
			lv2:scalePoint [ rdfs:label "off" ; rdf:value 0 ] ;
			)
	, TTF_IPORT(9, "note9", "9th Note",  -24, 24,  0,
			lv2:portProperty lv2:integer;
			lv2:scalePoint [ rdfs:label "off" ; rdf:value 0 ] ;
			)
	, TTF_IPORT(10, "note10", "10th Note",  -24, 24,  0,
			lv2:portProperty lv2:integer;
			lv2:scalePoint [ rdfs:label "off" ; rdf:value 0 ] ;
			)
	, TTF_IPORT(11, "note11", "11th Note",  -24, 24,  0,
			lv2:portProperty lv2:integer;
			lv2:scalePoint [ rdfs:label "off" ; rdf:value 0 ] ;
			)
	; rdfs:comment "Similar to MIDI Chord harmonizer, this allows to create chromatic note clusters." ;
	.

#elif defined MX_CODE

#define MAX_CLUSTER 11

static inline void filter_chromaticcluster_noteon(MidiFilter* self, uint32_t tme, uint8_t chn, int note, uint8_t vel) {
	uint8_t buf[3];
	if (!midi_valid(note)) return;
	buf[0] = MIDI_NOTEON | chn;
	buf[1] = note;
	buf[2] = vel;
	self->memCS[chn][note]++;
	if (self->memCS[chn][note] == 1) {
		forge_midimessage(self, tme, buf, 3);
	}
}

static inline void filter_chromaticcluster_noteoff(MidiFilter* self, uint32_t tme, uint8_t chn, int note, uint8_t vel) {
	uint8_t buf[3];
	if (!midi_valid(note)) return;
	buf[0] = MIDI_NOTEOFF | chn;
	buf[1] = note;
	buf[2] = vel;
	if (self->memCS[chn][note] > 0) {
		self->memCS[chn][note]--;
		if (self->memCS[chn][note] == 0)
			forge_midimessage(self, tme, buf, 3);
	}
}

static inline void filter_chromaticcluster_panic(MidiFilter* self, const uint8_t c, const uint32_t tme) {
	int k;
	for (k=0; k < 127; ++k) {
		if (self->memCS[c][k] > 0) {
			uint8_t buf[3];
			buf[0] = MIDI_NOTEOFF | c;
			buf[1] = k;
			buf[2] = 0;
			forge_midimessage(self, tme, buf, 3);
		}
		self->memCI[c][k] = 0; // current cluster for this key
		self->memCS[c][k] = 0; // count note-on per key
		self->memCM[c][k] = 0; // last known velocity for this key
	}
}

static void
filter_midi_chromaticcluster(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size)
{
	int i;
	const int chs = midi_limit_chn(floorf(*self->cfg[0]) -1);

	const uint8_t chn = buffer[0] & 0x0f;
	uint8_t       mst = buffer[0] & 0xf0;

	if (midi_is_panic(buffer, size)) {
		filter_chromaticcluster_panic(self, chn, tme);
	}

	if (size != 3
			|| !(mst == MIDI_NOTEON || mst == MIDI_NOTEOFF || mst == MIDI_POLYKEYPRESSURE)
			|| !(floorf(*self->cfg[0]) == 0 || chs == chn)
			)
	{
		forge_midimessage(self, tme, buffer, size);
		return;
	}

#if 0
	int64_t cluster = 0;
	for (i = 0; i < MAX_CLUSTER ; ++i) {
		int off = rintf(*(self->cfg[1 + i])); /* range -24 .. + 24 */
		/* map to range 0 .. 2^6 (64) and bitshift */
		off = (off + 32) >> (i*6);
		cluster |= off;
	}
#endif

	const uint8_t key = buffer[1] & 0x7f;
	const uint8_t vel = buffer[2] & 0x7f;

	switch (mst) {
		case MIDI_NOTEON:
			self->memCI[chn][key] = 1; //cluster;
			self->memCM[chn][key] = vel;
			filter_chromaticcluster_noteon(self, tme, chn, key, vel);
			for (i = 0; i < MAX_CLUSTER ; ++i) {
				int off = rintf(*(self->cfg[1 + i]));
				if (off != 0 && midi_valid (off + key)) {
					filter_chromaticcluster_noteon(self, tme, chn, key + off, vel);
				}
			}
			break;
		case MIDI_NOTEOFF:
			filter_chromaticcluster_noteoff(self, tme, chn, key, vel);
			for (i = 0; i < MAX_CLUSTER ; ++i) {
				int off = rintf(*(self->cfg[1 + i]));
				if (off != 0 && midi_valid (off + key)) {
					filter_chromaticcluster_noteoff(self, tme, chn, key + off, vel);
				}
			}
			self->memCI[chn][key] = 0;
			self->memCM[chn][key] = 0;
			break;
		case MIDI_POLYKEYPRESSURE:
			forge_midimessage(self, tme, buffer, size);
			for (i = 0; i < MAX_CLUSTER ; ++i) {
				int off = rintf(*(self->cfg[1 + i]));
				int note = key + off;
				if (off == 0 || !midi_valid (note)) {
					continue;
				}
				uint8_t buf[3];
				buf[0] = buffer[0];
				buf[1] = note;
				buf[2] = buffer[2];
				forge_midimessage(self, tme, buf, size);
			}
			break;
	}
}

static void filter_preproc_chromaticcluster(MidiFilter* self) {

	int c,k,i;
	int identical_cfg = 1;

	for (i = 0; i < MAX_CLUSTER; ++i) {
		if (floorf(self->lcfg[i+1]) != floorf(*self->cfg[i+1])) {
			identical_cfg = 0;
		}
	}
	if (identical_cfg) return;

	for (i = 0; i < MAX_CLUSTER ; ++i) {
		int off_o = rintf((self->lcfg[1 + i]));
		int off_n = rintf(*(self->cfg[1 + i]));

		if (off_n == off_o) {
			continue;
		}

		for (c=0; c < 16; ++c) {
			for (k=0; k < 127; ++k) {
				if (self->memCM[c][k] == 0) continue;
				if (self->memCI[c][k] == 0) continue;

				const uint8_t vel = self->memCM[c][k];

				int note = k + off_o;

				if (off_o != 0 && midi_valid (note)) {
					filter_chromaticcluster_noteoff(self, 0, c, note, 0);
				}

				note = k + off_n;
				if (off_n != 0 && midi_valid (note)) {
					filter_chromaticcluster_noteon(self, 0, c, note, vel);
				}
			}
		}
	}
}

static void filter_init_chromaticcluster(MidiFilter* self) {
	int c,k;
	for (c = 0; c < 16; ++c) for (k = 0; k < 127; ++k) {
		self->memCI[c][k] = 0; // current cluster for this key
		self->memCS[c][k] = 0; // count note-on per key
		self->memCM[c][k] = 0; // last known velocity for this key
	}
	self->preproc_fn = filter_preproc_chromaticcluster;
}

#endif
