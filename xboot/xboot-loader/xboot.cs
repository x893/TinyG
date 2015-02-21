using System;
using System.Collections.Generic;
using System.Text;

namespace xboot_loader
{
	public class XCommand
	{
		public byte[] Command { get; set; }
		public byte[] Data { get; set; }
		public int DataIndex { get; set; }
		public int DataCount { get; set; }
		public int ResponseLength { get; set; }
		public int ReplayIndex { get; set; }
		public XCommand(string cmd)
		{
			Command = Encoding.ASCII.GetBytes(cmd);
			ResponseLength = 1;
			ReplayIndex = 0;
		}
		public XCommand(string cmd, int replayIndex, int length)
			: this(cmd)
		{
			ResponseLength = length;
			ReplayIndex = replayIndex;
		}
	}

	class XBOOT
	{
		public const byte REPLY_ACK = (byte)'\r';
		public const byte REPLY_YES = (byte)'Y';
		public const byte REPLY_ERROR = (byte)'?';

		public static XCommand CMD_ENTER_PROG_MODE = new XCommand( "P" );
		public static XCommand CMD_PROGRAM_ID = new XCommand("S", -1, 7);
		public static XCommand CMD_CHIP_ERASE = new XCommand("e");
		public static XCommand CMD_CHECK_BLOCK_SUPPORT = new XCommand("b", 0, 3);
		public static XCommand CMD_SET_EXT_ADDRESS = new XCommand("Haaa");
		public static XCommand CMD_BLOCK_LOAD = new XCommand("BssF");
		public static XCommand CMD_BLOCK_READ = new XCommand("gssF", -1, 0);
		public static XCommand CMD_CRC = new XCommand("hCsss", 0, 3);
		public static XCommand CMD_CRC_WRITE = new XCommand("zsss");
		public static XCommand CMD_EXIT_BOOTLOADER = new XCommand("E");
	}
}
