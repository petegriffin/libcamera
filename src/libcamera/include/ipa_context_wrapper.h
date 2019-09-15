/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * ipa_context_wrapper.h - Image Processing Algorithm context wrapper
 */
#ifndef __LIBCAMERA_IPA_CONTEXT_WRAPPER_H__
#define __LIBCAMERA_IPA_CONTEXT_WRAPPER_H__

#include <ipa/ipa_interface.h>

namespace libcamera {

class IPAContextWrapper final : public IPAInterface
{
public:
	IPAContextWrapper(struct ipa_context *context);
	~IPAContextWrapper();

	int init() override;
	void configure(const std::map<unsigned int, IPAStream> &streamConfig,
		       const std::map<unsigned int, ControlInfoMap> &entityControls) override;

	void mapBuffers(const std::vector<IPABuffer> &buffers) override;
	void unmapBuffers(const std::vector<unsigned int> &ids) override;

	virtual void processEvent(const IPAOperationData &data) override;

private:
	static void queue_frame_action(void *ctx, unsigned int frame);
	static const struct ipa_callback_ops callbacks_;

	struct ipa_context *ctx_;
};

} /* namespace libcamera */

#endif /* __LIBCAMERA_IPA_CONTEXT_WRAPPER_H__ */
