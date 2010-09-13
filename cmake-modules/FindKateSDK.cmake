find_file( KATE_SDK kate/pluginconfigpageinterface.h )

if( KATE_SDK )
  message( STATUS "Found Kate development files" )
  set( KATE_SDK_FOUND YES )
endif( KATE_SDK )
