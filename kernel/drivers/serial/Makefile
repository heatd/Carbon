include build-helper.config

ifeq ($(KERN_IN_TREE_BUILD),)

# TODO: Add this

else

objs-$(CONFIG_SERIAL) += $(call DEFINE_OBJFILE,serial,serial.o)

module-name:=serial.ko

endif
