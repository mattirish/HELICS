# Federate flags

There are a number of flags which control how a federate acts with respect to timing and its interfaces.  The Timing flags and controls are described [here](./Timing.html).  There are also a number of other flags which control some aspects of the interfaces, and a few other flags which can be applied to specific interfaces.

### Federate flags

#### single_thread_federate

If specified in the federateInfo on creation this tells the core that this federate will only execute in a single thread and only a single federate is interacting with the connected core.

NOTE: This option is not fully enabled.

This disables the asynchronous functions in the federate and turns off a number of protection mechanisms for handling federate interaction across multiple threads.  This can be used for performance reasons and can interact with the Single_thread core types that are in development.

#### ignore_time_mismatch_warnings

if certain timing options are used this can cause the granted time to be greater than the requested time, with the period, or minTimeDelta specified.  This situation would normally generate a warning message, but if this option is enabled those warnings are silenced.

#### connections_required

When an interface requests a target it tries to find a match in the federation.  If it cannot find a match at the time the federation is initialized, then the default is to generate a warning.  This will not halt the federation but will display a log message.  If the connections required flag is set on a federate all subsequent `addTarget` calls on any interface will generate an error if the target is not available.  If the addTarget is made after the initialization point, the error is immediate.

#### connections_optional

When an interface requests a target it tries to find a match in the federation.  If it cannot find a match at the time the federation is initialized, then the default is to generate a warning.  This will not halt the federation but will display a log message.  If the connections_optional flag is set on a federate all subsequent `addTarget` calls on any interface will not generate any message if the target is not available.

#### strict_input_type_checking

This applies to Input interface.  If enabled this flag tells the inputs to check that the type matches.
