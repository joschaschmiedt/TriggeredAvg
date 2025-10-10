set(TRIGGERED_AVG_SOURCES_RELATIVE
    DataCollector.cpp
    GridDisplay.cpp
    MultiChannelRingBuffer.cpp
    OpenEphysLib.cpp
    PopupConfigurationWindow.cpp
    SinglePlotPanel.cpp
    TimeAxis.cpp
    TriggerSource.cpp
    TriggeredAvgActions.cpp
    TriggeredAvgCanvas.cpp
    TriggeredAvgEditor.cpp
    TriggeredAvgNode.cpp
)

set(TRIGGERED_AVG_HEADERS_RELATIVE
    DataCollector.h
    GridDisplay.h
    MultiChannelRingBuffer.h
    SinglePlotPanel.h
    TimeAxis.h
    TriggerSource.h
    TriggeredAvgActions.h
    TriggeredAvgCanvas.h
    TriggeredAvgEditor.h
    TriggeredAvgNode.h
)

# Convert to full paths
foreach(src ${TRIGGERED_AVG_SOURCES_RELATIVE})
    list(APPEND TRIGGERED_AVG_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${src})
endforeach()

foreach(hdr ${TRIGGERED_AVG_HEADERS_RELATIVE})
    list(APPEND TRIGGERED_AVG_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/${hdr})
endforeach()

# Set variables in parent scope
set(TRIGGERED_AVG_SOURCES ${TRIGGERED_AVG_SOURCES} PARENT_SCOPE)
set(TRIGGERED_AVG_HEADERS ${TRIGGERED_AVG_HEADERS} PARENT_SCOPE)
