# Header

| Length | Name           | Value        | Description                                               |
|--------|----------------|--------------|-----------------------------------------------------------|
| `6B`   | Header Magic   | "NYSIPC"     | Magic to identify the packet as belonging to the protocol |
| `1B`   | Version        | 1            | To expand the protocol at a later time                    |
| `2B`   | Echo           | `short`      | Echoed back to the client in the response                 |   
| `1B`   | Message Type   | MESSAGE_TYPE | Detailed below                                            |
| `2B`   | Message Length | `short`      | Length of message body (does not include header)          |
| `$`    | Message Body   | `any`        | Optional message body                                     |

# Message types
