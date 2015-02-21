using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.IO.Ports;
using System.Text;
using System.Threading;

namespace xboot_loader
{
	class Program
	{
		static UInt32 StartAddress = 0;
		static UInt32 EndAddress = 0;
		static UInt32 Size = 0;
		static UInt16 BlockSize = 0;

		static List<MemorySpecification> MemorySpecifications = new List<MemorySpecification>();

		static string DeviceName = string.Empty;
		static string PortName = string.Empty;
		static int PortSpeed = 0;

		static SerialPort Port = null;
		static Response Response = new Response();

		static void Main(string[] args)
		{
			int exitCode = 1;
			if (ParseOptions(args))
			{
				if (string.IsNullOrEmpty(PortName))
					Console.WriteLine("Port name not set");
				else
				{
					if (PortSpeed == 0)
						Console.WriteLine("Port speed not set");
					else if (MemorySpecifications.Count == 0)
						Console.WriteLine("No memory specification");
					else
					{
						exitCode = 0;
						foreach (MemorySpecification ms in MemorySpecifications)
						{
							if (!PrepareMemory(ms))
							{
								exitCode = 1;
								break;
							}
						}
						if (exitCode == 0)
						{
							try
							{
								Port = new SerialPort(PortName);
								Port.BaudRate = PortSpeed;
								Port.DataBits = 8;
								Port.Handshake = Handshake.None;
								Port.StopBits = StopBits.One;
								Port.Parity = Parity.None;
								Port.Open();
								if (SendCommand(XBOOT.CMD_ENTER_PROG_MODE).Status == ResponseStatus.Ack &&
									SendCommand(XBOOT.CMD_PROGRAM_ID).Status == ResponseStatus.Unknown &&
									Response.AnswerAsString() == "XBoot++")
								{
									exitCode = 0;
									foreach (MemorySpecification ms in MemorySpecifications)
									{
										if (!WriteMemory(ms))
										{
											exitCode = 1;
											break;
										}
									}
								}
								else
								{
									Console.WriteLine("XBoot not response");
								}
							}
							catch (Exception ex)
							{
								Console.WriteLine(string.Format("Exception:{0}", ex.Message));
								exitCode = 1;
							}
						}
						if (Port != null)
						{
							if (Port.IsOpen)
								Port.Close();
							Port = null;
						}
					}
				}
			}
			if (exitCode != 0)
				Console.ReadKey(true);
			Environment.Exit(exitCode);
		}

		private static bool SendStartAddress(uint address)
		{
			address >>= 1;
			XBOOT.CMD_SET_EXT_ADDRESS.Command[1] = (byte)((address >> 16) & 0xFF);
			XBOOT.CMD_SET_EXT_ADDRESS.Command[2] = (byte)((address >> 8) & 0xFF);
			XBOOT.CMD_SET_EXT_ADDRESS.Command[3] = (byte)((address >> 0) & 0xFF);
			if (SendCommand(XBOOT.CMD_SET_EXT_ADDRESS).Status != ResponseStatus.Ack)
			{
				Console.WriteLine("Set address error");
				return false;
			}
			return true;
		}

		private static bool WriteMemory(MemorySpecification ms)
		{
			bool result = false;

			if (SendCommand(XBOOT.CMD_CHIP_ERASE).Status != ResponseStatus.Ack)
			{
				Console.WriteLine("Chip erase error");
				return false;
			}
			if (SendCommand(XBOOT.CMD_CHECK_BLOCK_SUPPORT).Status != ResponseStatus.Yes)
			{
				Console.WriteLine("No block support");
				return false;
			}
			BlockSize = (UInt16)((UInt16)Response.Answer[1] << 8 | (UInt16)Response.Answer[2]);
			if (!SendStartAddress(StartAddress))
				return false;

			Console.WriteLine(" Write pass ----------");
			Console.Write("            ");
			int len = ms.Image.Length;
			int bsize;
			XBOOT.CMD_BLOCK_LOAD.Data = ms.Image;
			XBOOT.CMD_BLOCK_LOAD.DataIndex = 0;
			result = true;
			//! CRC16.Reset();
			int percent = 0, n_percent;
			while (len != 0)
			{
				bsize = (len >= BlockSize) ? BlockSize : len;
				len -= bsize;

				XBOOT.CMD_BLOCK_LOAD.Command[1] = (byte)((bsize >> 8) & 0xFF);
				XBOOT.CMD_BLOCK_LOAD.Command[2] = (byte)((bsize >> 0) & 0xFF);
				XBOOT.CMD_BLOCK_LOAD.DataCount = bsize;
				if (SendCommand(XBOOT.CMD_BLOCK_LOAD).Status != ResponseStatus.Ack)
				{
					Console.WriteLine("Block load error");
					result = false;
					break;
				}

				while (bsize-- != 0)
				{
					//! CRC16.Write(XBOOT.CMD_BLOCK_LOAD.Data[XBOOT.CMD_BLOCK_LOAD.DataIndex++]);
					XBOOT.CMD_BLOCK_LOAD.DataIndex++;
				}

				n_percent = 10 * XBOOT.CMD_BLOCK_LOAD.DataIndex / ms.Image.Length;
				if (percent != n_percent)
				{
					Console.Write("#");
					percent = n_percent;
				}
			}
			Console.WriteLine("");
			if (!result)
				return false;

			Console.WriteLine("Verify pass ----------");
			Console.Write("            ");
			if (!SendStartAddress(StartAddress))
				return false;

			len = ms.Image.Length;
			result = true;
			XBOOT.CMD_BLOCK_LOAD.DataIndex = 0;
			percent = 0;
			while (len != 0)
			{
				bsize = (len >= BlockSize) ? BlockSize : len;
				len -= bsize;

				XBOOT.CMD_BLOCK_READ.Command[1] = (byte)((bsize >> 8) & 0xFF);
				XBOOT.CMD_BLOCK_READ.Command[2] = (byte)((bsize >> 0) & 0xFF);
				XBOOT.CMD_BLOCK_READ.ResponseLength = bsize;
				if (SendCommand(XBOOT.CMD_BLOCK_READ).Status != ResponseStatus.Unknown)
				{
					Console.WriteLine(string.Format(" Block read error at {0}", XBOOT.CMD_BLOCK_LOAD.DataIndex));
					result = false;
					break;
				}
				int idx = 0;
				while (bsize-- != 0)
				{
					if (Response.Answer[idx++] != XBOOT.CMD_BLOCK_LOAD.Data[XBOOT.CMD_BLOCK_LOAD.DataIndex])
					{
						Console.WriteLine(string.Format(" Data not equal at {0:X} Origin:{1:X} Flash:{2:X}",
							XBOOT.CMD_BLOCK_LOAD.DataIndex,
							XBOOT.CMD_BLOCK_LOAD.Data[XBOOT.CMD_BLOCK_LOAD.DataIndex],
							Response.Answer[idx++]
							));
						result = false;
						break;
					}
					XBOOT.CMD_BLOCK_LOAD.DataIndex++;
				}
				if (!result)
					break;

				n_percent = 10 * XBOOT.CMD_BLOCK_LOAD.DataIndex / ms.Image.Length;
				if (percent != n_percent)
				{
					Console.Write("#");
					percent = n_percent;
				}
			}
			Console.WriteLine("");
			if (!result)
				return false;

			len = ms.Image.Length;

			XBOOT.CMD_CRC_WRITE.Command[1] = XBOOT.CMD_CRC.Command[2] = (byte)((len >> 16) & 0xFF);
			XBOOT.CMD_CRC_WRITE.Command[2] = XBOOT.CMD_CRC.Command[3] = (byte)((len >> 8) & 0xFF);
			XBOOT.CMD_CRC_WRITE.Command[3] = XBOOT.CMD_CRC.Command[4] = (byte)((len >> 0) & 0xFF);

			if (!SendStartAddress(StartAddress << 1))
				return false;

			if (SendCommand(XBOOT.CMD_CRC).Status != ResponseStatus.Ack)
			{
				Console.WriteLine("Get CRC error");
				return false;
			}

			UInt16 csum = (UInt16)((Response.Answer[1] << 8) | Response.Answer[2]);
//!			if (csum != CRC16.Read())
//!			{
//!				Console.WriteLine("CRC not equal");
//!				return false;
//!			}
			if (SendCommand(XBOOT.CMD_CRC_WRITE).Status != ResponseStatus.Ack)
			{
				Console.WriteLine("Write CRC error");
				return false;
			}
			if (SendCommand(XBOOT.CMD_EXIT_BOOTLOADER).Status != ResponseStatus.Ack)
			{
				Console.WriteLine("Exit bootloader error");
				return false;
			}

			return true;
		}

		private static bool PrepareMemory(MemorySpecification ms)
		{
			if (!string.IsNullOrEmpty(ms.FileName) && File.Exists(ms.FileName))
			{
				if ((ms.FileFormat == FileFormat.Binary && PrepareMemoryBin(ms)) ||
					(ms.FileFormat == FileFormat.Hex && PrepareMemoryHex(ms)))
				{
					return true;
				}
				else
				{
					Console.WriteLine(string.Format("Can't prepare memory: {0}", ms.FileName));
				}
			}
			return false;
		}

		const uint BLOCK_SIZE = 1024;
		static byte[] HexMemory = null;
		static uint HexMaxAddress = 0;
		static uint HexMinAddress = uint.MaxValue;

		private static void writeToMemory(uint address, byte data)
		{
			uint block = address / BLOCK_SIZE;
			uint maxSize = (block + 1) * BLOCK_SIZE;
			if (HexMemory == null)
			{
				HexMemory = new byte[maxSize];
			}
			else if (maxSize > HexMemory.Length)
			{
				byte[] nm = new byte[maxSize];
				HexMemory.CopyTo(nm, 0);
				for (int i = HexMemory.Length; i < maxSize; i++)
					nm[i] = 0xFF;

				HexMemory = nm;
			}
			HexMaxAddress = Math.Max(HexMaxAddress, address);
			HexMinAddress = Math.Min(HexMinAddress, address);
			HexMemory[address] = data;
		}

		private static bool PrepareMemoryHex(MemorySpecification ms)
		{
			bool result = false;
			try
			{
				using (FileStream fs = new FileStream(ms.FileName, FileMode.Open, FileAccess.Read))
				using (TextReader br = new StreamReader(fs))
				{
					result = true;
					bool fail = false;
					INTEL_COMMAND command = INTEL_COMMAND.EOF;
					string line;
					int lineNumber = 0;
					uint count = 0, address = 0;
					byte data = 0, checksum;
					UInt32 extendAddress = 0, startAddress = 0, segment_address = 0;
					int idx = 0;

					while ((line = br.ReadLine()) != null)
					{
						lineNumber++;
						line = line.Trim();
						if (line.Length == 0)
							continue;
						if (line.StartsWith("S"))
						{	// Motorola format
							Console.WriteLine("Motorola format not support");
							result = false;
							break;
						}
						if (line.StartsWith(":"))
						{	// Intel format
							if (line.Length < 11)
								fail = true;
							else
							{
								fail |= !uint.TryParse(line.Substring(1, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out count);
								fail |= !uint.TryParse(line.Substring(3, 4), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out address);
								fail |= !byte.TryParse(line.Substring(7, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out data);
								command = (INTEL_COMMAND)data;
								fail |= !byte.TryParse(line.Substring(line.Length - 2, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out checksum);
							}
							if (fail)
							{
								Console.WriteLine(string.Format("Can't parse line {0}", lineNumber));
								result = false;
								break;
							}
							switch (command)
							{
								case INTEL_COMMAND.EOF: // End of File
									break;
								case INTEL_COMMAND.DATA:
									idx = 9;
									goto data_loop;
								case INTEL_COMMAND.DATA_LOOP:
								data_loop:
									for (; !fail && count > 0; --count)
									{
										if (line.Length < idx + 2)
										{
											Console.WriteLine(string.Format("Data record too short at line {0}", lineNumber));
											fail = true;
										}
										else
											fail = !byte.TryParse(line.Substring(idx, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out data);

										if (!fail)
											writeToMemory(segment_address + extendAddress + address - StartAddress, data);

										address++;
										idx += 2;
									}
									break;
								case INTEL_COMMAND.EXT_SEGMENT_ADDR: // Extended Segment Address Record
									#region Extended segment address record
									if (count != 2 || line.Length != 15)
									{
										Console.WriteLine(string.Format("Bad Extended segment address record line {0}.", lineNumber));
									}
									else
									{
										fail |= !uint.TryParse(line.Substring(9, 4), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out segment_address);
										if (fail)
										{
											result = false;
											Console.WriteLine(string.Format("Bad Extended segment address record line {0}.", lineNumber));
										}
										else
											segment_address <<= 4;
									}
									break;
									#endregion
								case INTEL_COMMAND.SEGMENT_ADDR:
									#region Start Segment Address Record
									if (count != 4)
									{
										Console.WriteLine(string.Format("Bad Start Segment record line {0}.", lineNumber));
									}
									else
									{
										fail |= !uint.TryParse(line.Substring(9, 8), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out startAddress);
										if (fail)
										{
											result = false;
											Console.WriteLine(string.Format("Bad Start Segment records line {0}.", lineNumber));
										}
									}
									break;
									#endregion
								case INTEL_COMMAND.EXTEND_ADDR:
									#region Extended Linear Address Record
									if (line.Length != 15)
									{
										Console.WriteLine(string.Format("Bad Extended Address record line {0}.", lineNumber));
										fail = true;
									}
									else
									{
										fail |= !uint.TryParse(line.Substring(9, 4), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out extendAddress);
										if (fail)
										{
											result = false;
											Console.WriteLine(string.Format("Bad Extended Address record line {0}.", lineNumber));
										}
										else
											extendAddress = extendAddress << 16;
									}
									break;
									#endregion
								case INTEL_COMMAND.LINEAR_ADDR:
									#region Start Linear Address Record
									if (count != 4)
									{
										Console.WriteLine(string.Format("Bad Start Address record line {0}.", lineNumber));
									}
									else
									{
										fail |= !uint.TryParse(line.Substring(9, 8), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out startAddress);
										if (fail)
										{
											result = false;
											Console.WriteLine(string.Format("Bad Start Address record line {0}.", lineNumber));
										}
									}
									break;
									#endregion
								default:
									Console.WriteLine(string.Format("Bad command {0} at line {1}.", command, lineNumber));
									fail = true;
									result = false;
									break;
							}
							if (fail)
								break;
						}

					}
					br.Close();
					fs.Close();
				}
			}
			catch (Exception ex)
			{
				Console.WriteLine(string.Format("PrepareMemory: {0}", ex.Message));
			}
			if (result && HexMaxAddress >= HexMinAddress)
			{
				StartAddress = HexMinAddress;
				uint i = HexMaxAddress - HexMinAddress + 1;
				byte[] image = new byte[i];
				while (i != 0)
				{
					--i;
					image[i] = HexMemory[i];
				}
				ms.Image = image;
			}
			return result;
		}

		private static bool PrepareMemoryBin(MemorySpecification ms)
		{
			bool result = false;
			try
			{
				using (FileStream fs = new FileStream(ms.FileName, FileMode.Open, FileAccess.Read))
				using (BinaryReader br = new BinaryReader(fs))
				{
					int len = (int)fs.Length;
					byte[] image = new byte[len];
					if (len == br.Read(image, 0, len))
					{
						ms.Image = image;
						result = true;
					}
					br.Close();
					fs.Close();
				}
			}
			catch (Exception ex)
			{
				Console.WriteLine(string.Format("PrepareMemory: {0}", ex.Message));
			}
			return result;
		}

		private static Response SendCommand(XCommand cmd)
		{
			Port.DiscardInBuffer();
			Port.DiscardOutBuffer();
			Port.Write(cmd.Command, 0, cmd.Command.Length);
			if (cmd.Data != null && cmd.DataCount > 0 && cmd.Data.Length >= (cmd.DataIndex + cmd.DataCount))
				Port.Write(cmd.Data, cmd.DataIndex, cmd.DataCount);
			int timeout = 200;
			while (timeout != 0 && Port.BytesToRead < cmd.ResponseLength)
			{
				--timeout;
				Thread.Sleep(10);
			}
			if (Port.BytesToRead >= cmd.ResponseLength)
			{
				byte[] bytes = new byte[Port.BytesToRead];
				int readed = Port.Read(bytes, 0, Port.BytesToRead);
				Response.Status = ResponseStatus.Unknown;
				if (cmd.ReplayIndex >= 0)
				{
					byte replay = bytes[cmd.ReplayIndex];
					if (replay == XBOOT.REPLY_ACK)
						Response.Status = ResponseStatus.Ack;
					else if (replay == XBOOT.REPLY_YES)
						Response.Status = ResponseStatus.Yes;
					else if (replay == XBOOT.REPLY_ERROR)
						Response.Status = ResponseStatus.Error;
				}
				byte[] answer = new byte[cmd.ResponseLength];
				for (int i = 0; i < answer.Length; i++)
					answer[i] = bytes[i];
				Response.Answer = answer;
			}
			else
				Response.Status = ResponseStatus.Timeout;
			return Response;
		}

		static void port_DataReceived(object sender, SerialDataReceivedEventArgs e)
		{
			SerialPort port = sender as SerialPort;
			if (port != null && port.BytesToRead > 0)
			{
				byte[] buffer = new byte[port.BytesToRead];
				int bytes = port.Read(buffer, 0, port.BytesToRead);

			}
		}

		#region ParseOptions 
		private static bool ParseOptions(string[] args)
		{
			string opt = string.Empty;
			bool param = false;
			bool error = false;
			for(int idx = 0; idx < args.Length; idx++)
			{
				string arg = args[idx];
				if (arg.StartsWith("-"))
				{
					opt = arg;
					param = false;
				}
				switch (opt)
				{
					case "-p":
						#region Set device type
						if (param)
						{
							DeviceName = arg;
							if (DeviceName == "x256a3")
							{
								StartAddress = 0;
								Size = (256 + 8) * 1024;
								EndAddress = StartAddress + Size - 1;
							}
							param = false;
						}
						else
							param = true;
						break;
						#endregion
					case "-P":
						#region Set port name
						if (param)
						{
							PortName = arg.ToUpperInvariant();
							param = false;
						}
						else
							param = true;
						break;
						#endregion
					case "-b":
						#region Set port speed
						if (param)
						{
							if (!int.TryParse(arg, out PortSpeed))
							{
								Console.WriteLine("Bad port speed value");
								error = true;
							}
							param = false;
						}
						else
							param = true;
						break;
						#endregion
					case "-U":
						#region Memory operation specification
						if (param)
						{
							MemorySpecification ms = new MemorySpecification();

							if (arg.StartsWith("flash:"))
								ms.MemoryType = MemoryType.Flash;
							else if (arg.StartsWith("eeprom:"))
								ms.MemoryType = MemoryType.EEPROM;
							else if (arg.StartsWith("usersig:"))
								ms.MemoryType = MemoryType.UserSig;

							if (ms.MemoryType == MemoryType.Unknown)
							{
								Console.WriteLine(string.Format("Bad memory type {0}", arg));
								error = true;
							}
							else
							{
								int i = arg.IndexOf(":");
								if (i < arg.Length - 1)
								{
									arg = arg.Substring(i + 1);
									if (arg.StartsWith("w:"))
										ms.MemoryOpearion = MemoryOpearion.Write;
									else if (arg.StartsWith("r:"))
										ms.MemoryOpearion = MemoryOpearion.Read;
									else if (arg.StartsWith("v:"))
										ms.MemoryOpearion = MemoryOpearion.Verify;
									if (ms.MemoryOpearion == MemoryOpearion.Unknown)
									{
										Console.WriteLine(string.Format("Bad memory operation {0}", arg));
										error = true;
									}
									else
									{
										i = arg.IndexOf(":");
										if (i < arg.Length - 1)
										{
											ms.FileName = arg.Substring(i + 1);
											if (File.Exists(ms.FileName))
											{
												FileInfo fi = new FileInfo(ms.FileName);
												switch (fi.Extension.ToUpperInvariant())
												{
													case ".BIN":
														ms.FileFormat = FileFormat.Binary;
														break;
													case ".HEX":
														ms.FileFormat = FileFormat.Hex;
														break;
												}
												if (ms.FileFormat == FileFormat.Unknown)
												{
													Console.WriteLine(string.Format("Unknown file format {0}", ms.FileName));
													error = true;
												}
												else
												{
													MemorySpecifications.Add(ms);
												}
											}
											else
											{
												Console.WriteLine(string.Format("File not exist {0}", ms.FileName));
												error = true;
											}
										}
										else
										{
											Console.WriteLine(string.Format("Bad file name {0}", arg));
											error = true;
										}
									}
								}
								else
								{
									Console.WriteLine(string.Format("Bad memory opearion {0}", arg));
									error = true;
								}
							}
							param = false;
						}
						else
							param = true;
						break;
						#endregion
					case "-start":
						if (param)
						{
							if (!ParseIntValue(arg, out StartAddress))
							{
								Console.WriteLine("Bad start address");
								error = true;
							}
						}
						else
							param = true;
						break;
					case "-end":
						if (param)
						{
							if (!ParseIntValue(arg, out EndAddress))
							{
								Console.WriteLine("Bad end address");
								error = true;
							}
						}
						else
							param = true;
						break;
					case "-size":
						if (param)
						{
							if (!ParseIntValue(arg, out Size))
							{
								Console.WriteLine("Bad size");
								error = true;
							}
						}
						else
						{
							param = true;
						}
						break;
					default:
						Console.WriteLine(string.Format("Unknown option: {0}", opt));
						break;
				}
				if (!param)
					opt = string.Empty;
			}
			return !error;
		}
		#endregion

		#region ParseIntValue 
		private static bool ParseIntValue(string arg, out UInt32 value)
		{
			arg = arg.ToUpperInvariant();
			value = 0;
			if (arg.StartsWith("0X"))
			{

				if (UInt32.TryParse(arg.Substring(2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out value))
					return true;
			}
			else if (arg.EndsWith("K"))
			{
				if (arg.Length >= 2 && UInt32.TryParse(arg.Substring(2), NumberStyles.Integer, CultureInfo.InvariantCulture, out value))
					return true;
			}
			else
				if (UInt32.TryParse(arg, NumberStyles.Integer, CultureInfo.InvariantCulture, out value))
				{
					value = value * 1024;
					return true;
				}
			return false;
		}
		#endregion
	}

	enum MemoryType
	{
		Unknown,
		Flash,
		EEPROM,
		UserSig
	}
	enum MemoryOpearion
	{
		Unknown,
		Write,
		Read,
		Verify
	}
	enum FileFormat
	{
		Unknown,
		Hex,
		Binary
	}

	enum ResponseStatus
	{
		Ack,
		Yes,
		Error,
		Timeout,
		Unknown
	}
	class Response
	{
		public ResponseStatus Status { get; set; }
		public byte[] Answer { get; set; }

		public string AnswerAsString()
		{
			if (Answer!=null && Answer.Length>0)
				return Encoding.ASCII.GetString(Answer, 0, Answer.Length);
			return string.Empty;
		}
	}

	class MemorySpecification
	{
		public string FileName { get; set; }
		public MemoryType MemoryType { get; set; }
		public MemoryOpearion MemoryOpearion { get; set; }
		public FileFormat FileFormat { get; set; }
		public byte[] Image { get; set; }

		public MemorySpecification()
		{
			MemoryType = MemoryType.Unknown;
			MemoryOpearion = MemoryOpearion.Unknown;
			FileFormat = FileFormat.Unknown;
			FileName = string.Empty;
			Image = null;
		}
	}

	enum INTEL_COMMAND : byte
	{
		UNKNOWN = 0xFF,
		DATA = 0x00,
		EOF = 0x01,
		EXT_SEGMENT_ADDR = 0x02,
		SEGMENT_ADDR = 0x03,
		EXTEND_ADDR = 0x04,
		LINEAR_ADDR = 0x05,
		DATA_LOOP = 0x10
	}
}
