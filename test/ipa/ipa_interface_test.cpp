/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * ipa_interface_test.cpp - Test the IPA interface
 */

#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <libcamera/event_dispatcher.h>
#include <libcamera/event_notifier.h>
#include <libcamera/timer.h>

#include <ipa/ipa_vimc.h>

#include "device_enumerator.h"
#include "ipa_manager.h"
#include "ipa_module.h"
#include "pipeline_handler.h"
#include "test.h"
#include "thread.h"

using namespace std;
using namespace libcamera;

class IPAInterfaceTest : public Test, public Object
{
public:
	IPAInterfaceTest()
		: trace_(IPAOperationNone), notifier_(nullptr), fd_(-1)
	{
	}

	~IPAInterfaceTest()
	{
		delete notifier_;
	}

protected:
	int init() override
	{
		/* Create a pipeline handler for vimc. */
		std::vector<PipelineHandlerFactory *> &factories =
			PipelineHandlerFactory::factories();
		for (PipelineHandlerFactory *factory : factories) {
			if (factory->name() == "PipelineHandlerVimc") {
				pipe_ = factory->create(nullptr);
				break;
			}
		}

		if (!pipe_) {
			cerr << "Vimc pipeline not found" << endl;
			return TestPass;
		}

		/* Create and open the communication FIFO. */
		int ret = mkfifo(VIMC_IPA_FIFO_PATH, S_IRUSR | S_IWUSR);
		if (ret) {
			ret = errno;
			cerr << "Failed to create IPA test FIFO at '"
			     << VIMC_IPA_FIFO_PATH << "': " << strerror(ret)
			     << endl;
			return TestFail;
		}

		ret = open(VIMC_IPA_FIFO_PATH, O_RDONLY | O_NONBLOCK);
		if (ret < 0) {
			ret = errno;
			cerr << "Failed to open IPA test FIFO at '"
			     << VIMC_IPA_FIFO_PATH << "': " << strerror(ret)
			     << endl;
			unlink(VIMC_IPA_FIFO_PATH);
			return TestFail;
		}
		fd_ = ret;

		notifier_ = new EventNotifier(fd_, EventNotifier::Read, this);
		notifier_->activated.connect(this, &IPAInterfaceTest::readTrace);

		return TestPass;
	}

	int run() override
	{
		EventDispatcher *dispatcher = thread()->eventDispatcher();
		Timer timer;

		ipa_ = IPAManager::instance()->createIPA(pipe_.get(), 0, 0);
		if (!ipa_) {
			cerr << "Failed to create VIMC IPA interface" << endl;
			return TestFail;
		}

		/* Test initialization of IPA module. */
		ipa_->init();
		timer.start(1000);
		while (timer.isRunning() && trace_ != IPAOperationInit)
			dispatcher->processEvents();

		if (trace_ != IPAOperationInit) {
			cerr << "Failed to test IPA initialization sequence"
			     << endl;
			return TestFail;
		}

		return TestPass;
	}

	void cleanup() override
	{
		close(fd_);
		unlink(VIMC_IPA_FIFO_PATH);
	}

private:
	void readTrace(EventNotifier *notifier)
	{
		ssize_t s = read(notifier->fd(), &trace_, sizeof(trace_));
		if (s < 0) {
			int ret = errno;
			cerr << "Failed to read from IPA test FIFO at '"
			     << VIMC_IPA_FIFO_PATH << "': " << strerror(ret)
			     << endl;
			trace_ = IPAOperationNone;
		}
	}

	std::shared_ptr<PipelineHandler> pipe_;
	std::unique_ptr<IPAInterface> ipa_;
	enum IPAOperationCode trace_;
	EventNotifier *notifier_;
	int fd_;
};

TEST_REGISTER(IPAInterfaceTest)
