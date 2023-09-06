    .section ".rodata"
    .balign 4
    .global help_msg
help_msg:
    .incbin "Source/help.txt"
    .zero 1
    .set _sizeof_help_msg, . - help_msg
    .global help_size
help_size:
    .long _sizeof_help_msg
    .section ".text"
