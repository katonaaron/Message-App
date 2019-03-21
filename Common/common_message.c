#include "common_message.h"

//TODO: 
//      check OP_CODE
//      rename Message parameter

static CM_ERROR CreateMessage(CM_SIZE BufferSize, CM_OP_CODE Operation, CM_MESSAGE** Message)
{
    if (BufferSize < 0 || NULL == Message)
    {
        return CM_INVALID_PARAMETER;
    }

    CM_MESSAGE* message = NULL;
    CM_SIZE messageBufferSize;

    if (BufferSize > MAX_MESSAGE_BUFFER_SIZE)
    {
        messageBufferSize = MAX_MESSAGE_BUFFER_SIZE;
    }
    else
    {
        messageBufferSize = BufferSize;
    }

    message = (CM_MESSAGE*)malloc(GET_MESSAGE_SIZE(messageBufferSize));
    if (NULL == message)
    {
        *Message = NULL;
        return CM_NO_MEMORY;
    }

    message->Operation = Operation;
    message->Parts = BufferSize / MAX_MESSAGE_BUFFER_SIZE + 1;
    message->TotalSize = BufferSize;

    message->Size = messageBufferSize;
    message->Buffer[0] = 0;

    *Message = message;
    return CM_SUCCESS;
}

static CM_ERROR CopyMessage(const CM_MESSAGE* SourceMessage, DWORD Offset, CM_MESSAGE** NewMessage)
{
    if (NULL == SourceMessage || NULL == NewMessage)
    {
        return CM_SUCCESS;
    }

    CM_MESSAGE* message = *NewMessage;

    if (NULL == message)
    {
        message = (CM_MESSAGE*)malloc(GET_MESSAGE_SIZE(SourceMessage->TotalSize));
        if (NULL == message)
        {
            return CM_NO_MEMORY;
        }

        message->Operation = SourceMessage->Operation;
        message->TotalSize = SourceMessage->TotalSize;
        message->Parts = 1;
        message->Size = 0;

        *NewMessage = message;
    }

    memcpy(message->Buffer + Offset, SourceMessage->Buffer, SourceMessage->Size);
    message->Size += SourceMessage->Size;
    return CM_SUCCESS;
}

CM_ERROR SendMessageToServer(CM_CLIENT * Client, void * Message, CM_SIZE MessageSize, CM_OP_CODE Operation)
{
    if (NULL == Client || (NULL == Message && MessageSize != 0) || MessageSize < 0)
    {
        return CM_INVALID_PARAMETER;
    }

    INT rollback = 0;
    CM_MESSAGE* message = NULL;

    CM_DATA_BUFFER* dataBuffer = NULL;
    CM_SIZE sendByteCount = 0;
    CM_ERROR error = CM_SUCCESS;

    error = CreateMessage(MessageSize, Operation, &message);
    if (error)
    {
        goto send_message_to_server_cleanup;
    }
    rollback = 1;

    error = CreateDataBuffer(&dataBuffer, GET_MESSAGE_SIZE(message->Size));
    if (error)
    {
        goto send_message_to_server_cleanup;
    }
    rollback = 2;

    for (DWORD i = 0; i < message->Parts; i++)
    {
        if (i != message->Parts - 1)
        {
            message->Size = MAX_MESSAGE_BUFFER_SIZE;
        }
        else
        {
            message->Size = MessageSize - i * MAX_MESSAGE_BUFFER_SIZE;
        }

        memcpy(message->Buffer, (BYTE*)Message + i * MAX_MESSAGE_BUFFER_SIZE, message->Size);

        error = CopyDataIntoBuffer(dataBuffer, (const CM_BYTE*)message, GET_MESSAGE_SIZE(message->Size));
        if (error)
        {
            goto send_message_to_server_cleanup;
        }


        error = SendDataToServer(Client, dataBuffer, &sendByteCount);
        if (error)
        {
            goto send_message_to_server_cleanup;
        }

    }


send_message_to_server_cleanup:
    switch (rollback)
    {
    case 2:
        DestroyDataBuffer(dataBuffer);
    case 1:
        free(message);
    default:
        break;
    }
    return error;
}

CM_ERROR SendMessageToClient(CM_SERVER_CLIENT * Client, void * Message, CM_SIZE MessageSize, CM_OP_CODE Operation)
{
    if (NULL == Client || NULL == Message || MessageSize < 0)
    {
        return CM_INVALID_PARAMETER;
    }

    INT rollback = 0;
    CM_MESSAGE* message = NULL;

    CM_DATA_BUFFER* dataBuffer = NULL;
    CM_SIZE sendByteCount = 0;
    CM_ERROR error = CM_SUCCESS;

    error = CreateMessage(MessageSize, Operation, &message);
    if (error)
    {
        goto send_message_to_client_cleanup;
    }
    rollback = 1;

    error = CreateDataBuffer(&dataBuffer, MAX_MESSAGE_BUFFER_SIZE);
    if (error)
    {
        goto send_message_to_client_cleanup;
    }
    rollback = 2;

    for (DWORD i = 0; i < message->Parts; i++)
    {
        if (i != message->Parts - 1)
        {
            message->Size = MAX_MESSAGE_BUFFER_SIZE;
        }
        else
        {
            message->Size = MessageSize - i * MAX_MESSAGE_BUFFER_SIZE;
        }

        memcpy(message->Buffer, (BYTE*)Message + i * MAX_MESSAGE_BUFFER_SIZE, message->Size);

        error = CopyDataIntoBuffer(dataBuffer, (const CM_BYTE*)message, GET_MESSAGE_SIZE(message->Size));
        if (error)
        {
            goto send_message_to_client_cleanup;
        }

        error = SendDataToClient(Client, dataBuffer, &sendByteCount);
        if (error)
        {
            goto send_message_to_client_cleanup;
        }

    }


send_message_to_client_cleanup:
    switch (rollback)
    {
    case 2:
        DestroyDataBuffer(dataBuffer);
    case 1:
        free(message);
    default:
        break;
    }
    return error;
}

CM_ERROR ReceiveMessageFromServer(CM_CLIENT * Client, CM_MESSAGE ** Message)
{
    if (NULL == Client || NULL == Message)
    {
        return CM_INVALID_PARAMETER;
    }

    INT rollback = 0;
    CM_MESSAGE *receivedMessage = NULL;

    CM_DATA_BUFFER* dataBuffer = NULL;
    CM_SIZE receivedByteCount = 0;
    CM_ERROR result = CM_SUCCESS;

    *Message = NULL;

    result = CreateDataBuffer(&dataBuffer, MAX_MESSAGE_SIZE);
    if (result)
    {
        goto cleanup;
    }
    rollback = 1;

    result = ReceiveDataFormServer(Client, dataBuffer, &receivedByteCount);
    if (result)
    {
        goto cleanup;
    }
    
    CM_MESSAGE* message = (CM_MESSAGE*)malloc(dataBuffer->UsedBufferSize);
    if (NULL == message)
    {
        result = CM_NO_MEMORY;
        goto cleanup;
    }
    memcpy(message, dataBuffer->DataBuffer, dataBuffer->UsedBufferSize);
    *Message = message;

cleanup:
    switch (rollback)
    {
    case 1:
        DestroyDataBuffer(dataBuffer);
    default:
        break;
    }
    return result;
}

CM_ERROR ReceiveMessageFromClient(CM_SERVER_CLIENT * Client, CM_MESSAGE ** Message)
{
    if (NULL == Client || NULL == Message)
    {
        return CM_INVALID_PARAMETER;
    }

    INT rollback = 0;
    CM_MESSAGE* message = NULL, *receivedMessage = NULL;

    CM_DATA_BUFFER* dataBuffer = NULL;
    CM_SIZE receivedByteCount = 0;
    CM_ERROR error = CM_SUCCESS;

    error = CreateDataBuffer(&dataBuffer, MAX_MESSAGE_SIZE);
    if (error)
    {
        goto receive_message_from_client_cleanup;
    }
    rollback = 1;

    DWORD i = 0;
    do
    {
        error = ReceiveDataFromClient(Client, dataBuffer, &receivedByteCount);
        if (error)
        {
            goto receive_message_from_client_cleanup;
        }

        receivedMessage = (CM_MESSAGE*)dataBuffer->DataBuffer;
        error = CopyMessage(receivedMessage, i * MAX_MESSAGE_BUFFER_SIZE, &message);
        if (error)
        {
            goto receive_message_from_client_cleanup;
        }
        rollback = 2;

    } while (++i < receivedMessage->Parts);
    rollback = 1;

    *Message = message;

receive_message_from_client_cleanup:
    switch (rollback)
    {
    case 2:
        free(message);
        *Message = NULL;
    case 1:
        DestroyDataBuffer(dataBuffer);
    default:
        break;
    }
    return error;
}
