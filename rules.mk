
#################################
# silent rules
_v_       = @
_v_0      = $(_v_)
_v        = $(_v_$(V))

cc_v_     = @echo "  CC      $@";
cc_v_0    = $(cc_v_)
cc_v      = $(cc_v_$(V))

cxx_v_    = @echo "  CXX     $@";
cxx_v_0   = $(cxx_v_)
cxx_v     = $(cxx_v_$(V))

ccld_v_   = @echo "  CCLD    $@";
ccld_v_0  = $(ccld_v_)
ccld_v    = $(ccld_v_$(V))

cxxld_v_  = @echo "  CXXLD   $@";
cxxld_v_0 = $(cxxld_v_)
cxxld_v   = $(cxxld_v_$(V))

ar_v_     = @echo "  AR      $@";
ar_v_0    = $(ar_v_)
ar_v      = $(ar_v_$(V))

gen_v_    = @echo "  GEN     $@";
gen_v_0   = $(gen_v_)
gen_v     = $(gen_v_$(V))

devnull_v_  = >/dev/null
devnull_v_0 = $(devnull_v_)
devnull_v   = $(devnull_v_$(V))
#################################


DEL_EXTS = *.exe *.o *.obj *.a *.lib *.dll *.so *.exp

run_tests = for x in $$bins ; do \
	  echo $$x ; $(TEST_LIBPATH) ./$$x ; \
	  echo "return value: $$?" ; echo ; \
	done

run_tests_wine = for x in $$bins ; do \
	  echo $$x ; $(TEST_LIBPATH) WINEDEBUG="-all" wine ./$$x ; \
	  echo "return value: $$?" ; echo ; \
	done
