#ifndef _QTMOVIESOURCE_H
#define _QTMOVIESOURCE_H

#include <vector>
#include "qtmoviehelper.h"
#include "movieaudio.h"
#include "iointer.h"

class QTMovieSource : public ISource, public ITagParser {
    MovieAudioExtractionX m_session;
    AudioStreamBasicDescription m_description;
    AudioChannelLayoutX m_layout;
    SampleFormat m_format;
    std::vector<uint32_t> m_chanmap;
    bool m_extraction_complete;
    uint64_t m_duration, m_samples_read;
    std::map<uint32_t, std::wstring> m_tags;
public:
    explicit QTMovieSource(const std::wstring &path, bool alac_only=false);
    void setRange(int64_t start=0, int64_t length=0);
    uint64_t length() const { return m_duration; }
    const SampleFormat &getSampleFormat() const { return m_format; }
    const std::vector<uint32_t> *getChannelMap() const
    {
	return m_chanmap.size() ? &m_chanmap : 0;
    }
    size_t readSamples(void *buffer, size_t nsamples);
    const std::map<uint32_t, std::wstring> &getTags() const { return m_tags; }
    const std::vector<std::pair<std::wstring, int64_t> >
	*getChapters() const { return 0; }
    void setRenderQuality(uint32_t value)
    {
	m_session.setRenderQuality(value);
    }
};

#endif