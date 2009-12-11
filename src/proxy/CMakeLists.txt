# Builds the proxy  module

#set(QT_DONT_USE_QTGUI 1)
set(QT_USE_QTNETWORK 1)
set(QT_USE_QTDBUS 1)
include(${QT_USE_FILE})
include(FindProtobuf)
if (NOT PROTOBUF_FOUND)
       MESSAGE( FATAL_ERROR "Protobuf not found")
endif(NOT PROTOBUF_FOUND)
set(SOURCES
  backlogrequester.cpp
  chatlinemodel.cpp
  channelinfo.cpp
  chatlinemodelitem.cpp
  proxyconnection.cpp
  proxyuser.cpp
  protoconvert.cpp
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
  proxyconnection.h
  proxyuser.h
  proxyapplication.h
  proxymessageprocessor.h
  prototools.h
)

set(HEADERS
  protoconvert.h
)

WRAP_PROTO(PROTO_SRC protocol.proto)

qt4_wrap_cpp(MOC ${MOC_HDRS})

include_directories(${CMAKE_SOURCE_DIR}/src/common
		    ${CMAKE_SOURCE_DIR}/src/client
                    ${CMAKE_SOURCE_DIR}/src/proxy
		    ${PROTOBUF_INCLUDE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR})

add_library(mod_proxy STATIC ${SOURCES} ${MOC} ${HEADERS} ${PROTO_SRC})
add_dependencies(mod_proxy mod_common mod_client)