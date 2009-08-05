# Builds the qtui module

set(QT_DONT_USE_QTGUI 0)
set(QT_USE_QTNETWORK 1)
set(QT_USE_QTDBUS 1)
include(${QT_USE_FILE})
include(FindProtobuf)

set(SOURCES
  backlogrequester.cpp
  chatlinemodel.cpp
  channelinfo.cpp
  chatlinemodelitem.cpp
  proxy.cpp
  proxyapplication.cpp
  proxymessageprocessor.cpp
  connectionmanager.cpp
  prototools.cpp
)

set(MOC_HDRS
  backlogrequester.h
  chatlinemodel.h
  channelinfo.h
  connectionmanager.h
  proxy.h  
  proxyapplication.h
  proxymessageprocessor.h
  prototools.h
)

set(HEADERS
)

WRAP_PROTO(PROTO_SRC protocol.proto)

qt4_wrap_cpp(MOC ${MOC_HDRS} ${SPHDR})

include_directories(${PROTOBUF_INCLUDE_DIR}
		    ${CMAKE_SOURCE_DIR}/src/common
                    ${CMAKE_SOURCE_DIR}/src/proxy
                    ${CMAKE_CURRENT_BINARY_DIR}
		    )

add_library(mod_proxy STATIC ${SOURCES} ${SPSRC} ${MOC} ${DBUS} ${HEADERS} ${PROTO_SRC})
add_dependencies(mod_proxy mod_common)
