#ifndef _QTHELPER_H
#define _QTHELPER_H

#include <cstdio>
#include <QTML.h>
#include <cassert>
#include <Components.h>
#include "cfhelper.h"
#include "chanmap.h"
#include "win32util.h"

struct QTInitializer {
    QTInitializer(bool verbose) {
	DirectorySaver __saver__;
	if (verbose) std::fprintf(stderr, "initializing QTML...");
	InitializeQTML(0);
	if (verbose) std::fprintf(stderr, "done\n\n");
    }
    ~QTInitializer() { /*TerminateQTML();*/ }
};

inline ByteCount AudioChannelLayout_length(const AudioChannelLayout *acl)
{
    ByteCount n = offsetof(AudioChannelLayout, mChannelDescriptions);
    n += acl->mNumberChannelDescriptions * sizeof(AudioChannelDescription);
    return n;
}

class AudioChannelLayoutX {
public:
    typedef std::tr1::shared_ptr<AudioChannelLayout> owner_t;

    AudioChannelLayoutX() { create(0); }
    explicit AudioChannelLayoutX(size_t channel_count)
    {
	create(channel_count);
    }
    AudioChannelLayoutX(const AudioChannelLayout &layout)
    {
	attach(&layout);
    }
    AudioChannelLayoutX(const AudioChannelLayout *layout)
    {
	attach(layout);
    }
    operator AudioChannelLayout *() { return m_instance.get(); }
    operator const AudioChannelLayout *() const { return m_instance.get(); }
    AudioChannelLayout *operator->() { return m_instance.get(); }
    ByteCount size() { return AudioChannelLayout_length(m_instance.get()); }

    static AudioChannelLayoutX CreateBasic(unsigned nchannel)
    {
	AudioChannelLayoutX layout;
	static const uint32_t tab[] = {
	    kAudioChannelLayoutTag_Mono,
	    kAudioChannelLayoutTag_Stereo,
	    kAudioChannelLayoutTag_MPEG_3_0_A,
	    kAudioChannelLayoutTag_MPEG_4_0_A,
	    kAudioChannelLayoutTag_MPEG_5_0_A,
	    kAudioChannelLayoutTag_MPEG_5_1_A,
	    kAudioChannelLayoutTag_MPEG_6_1_A,
	    kAudioChannelLayoutTag_MPEG_7_1_C
	};
	assert(nchannel <= array_size(tab));
	layout->mChannelLayoutTag = tab[nchannel - 1];
	return layout;
    }
    static AudioChannelLayoutX FromChannelMap(const std::vector<uint32_t> &map)
    {
	AudioChannelLayoutX layout(map.size());
	layout->mChannelLayoutTag = GetChannelLayoutTagFromChannelMap(map);
	for (size_t i = 0; i < map.size(); ++i)
	    layout->mChannelDescriptions[i].mChannelLabel = map[i];
	return layout;
    }
private:
    void create(size_t channel_count)
    {
	size_t size = offsetof(AudioChannelLayout, mChannelDescriptions);
	size += channel_count * sizeof(AudioChannelDescription);
	m_instance.swap(owner_t(
		reinterpret_cast<AudioChannelLayout*>(xcalloc(1, size)),
		std::free));
	m_instance->mNumberChannelDescriptions = channel_count;
    }
    void attach(const AudioChannelLayout *layout)
    {
	ByteCount size = AudioChannelLayout_length(layout);
	owner_t p = owner_t(
	    reinterpret_cast<AudioChannelLayout*>(xmalloc(size)),
	    std::free);
	std::memcpy(p.get(), layout, size);
	m_instance.swap(p);
    }
    owner_t m_instance;
};

template <class T>
class PropertySupport {
    void _check(int err, const char *msg, OSType klass, OSType id)
    {
	if (err) {
	    std::string s = format("Error: %d: %s: %s/%s", err, msg,
		    fourcc(klass).svalue, fourcc(id).svalue);
	    throw std::runtime_error(s);
	}
    }
public:
    void getPropertyInfo(
	    OSType inPropClass,
	    OSType inPropID,
	    OSType *outPropType = 0,
	    ByteCount *outPropValueSize = 0,
	    UInt32 *outPropertyFlags = 0)
    {
	T *pT = static_cast<T*>(this);
	_check(pT->_getPropertyInfo(
			inPropClass,
			inPropID,
			outPropType,
			outPropValueSize,
			outPropertyFlags),
		"getPropertyInfo", inPropClass, inPropID);
    }
    void getProperty(
	    OSType inPropClass,
	    OSType inPropID,
	    ByteCount inPropValueSize,
	    void *outPropValueAddress,
	    ByteCount *outPropValueSizeUsed = 0)
    {
	T *pT = static_cast<T*>(this);
	_check(pT->_getProperty(
			inPropClass,
			inPropID,
			inPropValueSize,
			outPropValueAddress,
			outPropValueSizeUsed),
		"getProperty", inPropClass, inPropID);
    }
    void setProperty(
	    OSType inPropClass,
	    OSType inPropID,
	    ByteCount inPropValueSize,
	    const void * inPropValueAddress)
    {
	T *pT = static_cast<T*>(this);
	_check(pT->_setProperty(
			inPropClass,
			inPropID,
			inPropValueSize,
			inPropValueAddress),
		"setProperty", inPropClass, inPropID);
    }
    template <typename T>
    T getPodProperty(OSType cls, OSType id)
    {
	T data;
	getProperty(cls, id, sizeof(T), &data);
	return data;
    }
    template <typename T>
    void getPodProperty(OSType cls, OSType id, T *data)
    {
	getProperty(cls, id, sizeof(T), data);
    }
    template <typename T>
    void setPodProperty(OSType cls, OSType id, const T &value)
    {
	setProperty(cls, id, sizeof(T), &value);
    }
    template <typename T>
    void getVectorProperty(OSType cls, OSType id, std::vector<T> *result)
    {
	ByteCount size;
	getPropertyInfo(cls, id, 0, &size);
	std::vector<T> buffer(size / sizeof(T));
	if (size) getProperty(cls, id, size, &buffer[0]);
	result->swap(buffer);
    }
    /* for variable-sized struct */
    template <typename T>
    void getPointerProperty(
	    OSType cls,
	    OSType id,
	    std::tr1::shared_ptr<T> *result,
	    ByteCount *size = 0)
    {
	ByteCount sz;
	getPropertyInfo(cls, id, 0, &sz);
	std::tr1::shared_ptr<T> ptr(
		reinterpret_cast<T*>(xmalloc(sz)), std::free);
	getProperty(cls, id, sz, ptr.get());
	if (size) *size = sz;
	result->swap(ptr);
    }
};

class ComponentX: public PropertySupport<ComponentX> {
    typedef std::tr1::shared_ptr<ComponentInstanceRecord> owner_t;
    owner_t m_instance;
protected:
    void attach(ComponentInstance instance)
    {
	m_instance = owner_t(instance, CloseComponent);
    }
public:
    ComponentX() {}
    ComponentX(ComponentInstance instance)
	: m_instance(instance, CloseComponent) {}
    virtual ~ComponentX() {}
    static ComponentX OpenDefault(
	    OSType componentType, OSType componentSubType)
    {
	ComponentInstance ci;
	OSErr err = ::OpenADefaultComponent(
		componentType, componentSubType, &ci);
	if (err) {
	    std::string s = format("Error: %d: OpenADefaultComponent: %s/%s",
		    err,
		    fourcc(componentType).svalue,
		    fourcc(componentSubType).svalue);
	    throw std::runtime_error(s);
	}
	return ComponentX(ci);
    }
    operator ComponentInstance() { return m_instance.get(); }

    /* for PropertySupport */
    long _getPropertyInfo(
	    OSType inPropClass,
	    OSType inPropID,
	    OSType *outPropType,
	    ByteCount *outPropValueSize,
	    UInt32 *outPropertyFlags)
    {
	return QTGetComponentPropertyInfo(
		    m_instance.get(),
		    inPropClass,
		    inPropID,
		    outPropType,
		    outPropValueSize,
		    outPropertyFlags);
    }
    long _getProperty(
	    OSType inPropClass,
	    OSType inPropID,
	    ByteCount inPropValueSize,
	    void * outPropValueAddress,
	    ByteCount *outPropValueSizeUsed)
    {
	return QTGetComponentProperty(
		    m_instance.get(),
		    inPropClass,
		    inPropID,
		    inPropValueSize,
		    outPropValueAddress,
		    outPropValueSizeUsed);
    }
    long _setProperty(
	    OSType inPropClass,
	    OSType inPropID,
	    ByteCount inPropValueSize,
	    const void *inPropValueAddress)
    {
	return QTSetComponentProperty(
		    m_instance.get(),
		    inPropClass,
		    inPropID,
		    inPropValueSize,
		    inPropValueAddress);
    }
};

#endif