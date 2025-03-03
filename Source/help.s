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

    .global help_comp
help_comp:
    .incbin "Source/help.txt.compress"
    .zero 1
    .set _sizeof_help_comp, . - help_comp
    .global help_comp_size
help_comp_size:
    .long _sizeof_help_comp

    .section ".text"
