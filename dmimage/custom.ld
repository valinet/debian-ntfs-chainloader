SECTIONS
{
  . = 0x10000;
  ENTRY(_start)
  .text : {*(.text)}
}