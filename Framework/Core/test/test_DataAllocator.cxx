// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include "Framework/WorkflowSpec.h"
#include "Framework/DataProcessorSpec.h"
#include "Framework/runDataProcessing.h"
#include "Framework/DataAllocator.h"
#include "Framework/InputRecord.h"
#include "Framework/InputSpec.h"
#include "Framework/OutputSpec.h"
#include "Framework/ControlService.h"
#include "Framework/SerializationMethods.h"
#include "Headers/DataHeader.h"
#include "TestClasses.h"
#include "FairMQLogger.h"
#include <vector>

using DataProcessorSpec = o2::framework::DataProcessorSpec;
using WorkflowSpec = o2::framework::WorkflowSpec;
using ProcessingContext = o2::framework::ProcessingContext;
using OutputSpec = o2::framework::OutputSpec;
using InputSpec = o2::framework::InputSpec;
using Inputs = o2::framework::Inputs;
using Outputs = o2::framework::Outputs;
using AlgorithmSpec = o2::framework::AlgorithmSpec;
using InitContext = o2::framework::InitContext;
using ProcessingContext = o2::framework::ProcessingContext;
using DataRef = o2::framework::DataRef;
using DataRefUtils = o2::framework::DataRefUtils;
using ControlService = o2::framework::ControlService;

DataProcessorSpec getTimeoutSpec()
{
  // a timer process to terminate the workflow after a timeout
  auto processingFct = [](ProcessingContext& pc) {
    static int counter = 0;
    pc.allocator().snapshot(OutputSpec{ "TST", "TIMER", 0, OutputSpec::Timeframe }, counter);

    sleep(1);
    if (counter++ > 10) {
      LOG(ERROR) << "Timeout reached, the workflow seems to be broken";
      pc.services().get<ControlService>().readyToQuit(true);
    }
  };

  return DataProcessorSpec{ "timer",  // name of the processor
                            Inputs{}, // inputs empty
                            { OutputSpec{ "TST", "TIMER", 0, OutputSpec::Timeframe } },
                            AlgorithmSpec(processingFct) };
}

DataProcessorSpec getSourceSpec()
{
  auto processingFct = [](ProcessingContext& pc) {
    static int counter = 0;
    o2::test::TriviallyCopyable a(42, 23, 0xdead);
    o2::test::Polymorphic b(0xbeef);
    std::vector<o2::test::Polymorphic> c{ { 0xaffe }, { 0xd00f } };
    // class TriviallyCopyable is both messageable and has a dictionary, the default
    // picked by the framework is no serialization
    pc.allocator().snapshot(OutputSpec{ "TST", "MESSAGEABLE", 0, OutputSpec::Timeframe }, a);
    pc.allocator().snapshot(OutputSpec{ "TST", "MSGBLEROOTSRLZ", 0, OutputSpec::Timeframe },
                            o2::framework::ROOTSerialized<decltype(a)>(a));
    // class Polymorphic is not messageable, so the serialization type is deduced
    // from the fact that the type has a dictionary and can be ROOT-serialized.
    pc.allocator().snapshot(OutputSpec{ "TST", "ROOTNONTOBJECT", 0, OutputSpec::Timeframe }, b);
    // vector of ROOT serializable class
    pc.allocator().snapshot(OutputSpec{ "TST", "ROOTVECTOR", 0, OutputSpec::Timeframe }, c);
  };

  return DataProcessorSpec{ "source", // name of the processor
                            { InputSpec{ "timer", "TST", "TIMER", 0, InputSpec::Timeframe } },
                            { OutputSpec{ "TST", "MESSAGEABLE", 0, OutputSpec::Timeframe },
                              OutputSpec{ "TST", "MSGBLEROOTSRLZ", 0, OutputSpec::Timeframe },
                              OutputSpec{ "TST", "ROOTNONTOBJECT", 0, OutputSpec::Timeframe },
                              OutputSpec{ "TST", "ROOTVECTOR", 0, OutputSpec::Timeframe } },
                            AlgorithmSpec(processingFct) };
}

DataProcessorSpec getSinkSpec()
{
  auto processingFct = [](ProcessingContext& pc) {
    using DataHeader = o2::header::DataHeader;
    for (auto& input : pc.inputs()) {
      auto dh = o2::header::get<const DataHeader>(input.header);
      LOG(INFO) << dh->dataOrigin.str << " " << dh->dataDescription.str << " " << dh->payloadSize;
    }
    auto object1 = pc.inputs().get<o2::test::TriviallyCopyable>("input1");
    assert(*object1 == o2::test::TriviallyCopyable(42, 23, 0xdead));

    auto object4 = pc.inputs().get<o2::test::TriviallyCopyable>("input4");
    assert(*object4 == o2::test::TriviallyCopyable(42, 23, 0xdead));

    auto object2 = pc.inputs().get<o2::test::Polymorphic>("input2");
    assert(object2 != nullptr);
    assert(*(object2.get()) == o2::test::Polymorphic(0xbeef));

    auto object3 = pc.inputs().get<std::vector<o2::test::Polymorphic>>("input3");
    assert(object3 != nullptr);
    assert(object3->size() == 2);
    assert((*object3.get())[0] == o2::test::Polymorphic(0xaffe));
    assert((*object3.get())[1] == o2::test::Polymorphic(0xd00f));

    pc.services().get<ControlService>().readyToQuit(true);
  };

  return DataProcessorSpec{ "sink", // name of the processor
                            { InputSpec{ "input1", "TST", "MESSAGEABLE", 0, InputSpec::Timeframe },
                              InputSpec{ "input4", "TST", "MSGBLEROOTSRLZ", 0, InputSpec::Timeframe },
                              InputSpec{ "input2", "TST", "ROOTNONTOBJECT", 0, InputSpec::Timeframe },
                              InputSpec{ "input3", "TST", "ROOTVECTOR", 0, InputSpec::Timeframe } },
                            Outputs{},
                            AlgorithmSpec(processingFct) };
}

void defineDataProcessing(WorkflowSpec& specs)
{
  specs.emplace_back(getTimeoutSpec());
  specs.emplace_back(getSourceSpec());
  specs.emplace_back(getSinkSpec());
}
